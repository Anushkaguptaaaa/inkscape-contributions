/*
 *  PathStroke.cpp
 *  nlivarot
 *
 *  Created by fred on Tue Jun 17 2003.
 *
 */

#include "Path.h"
#include "Shape.h"
#include <libnr/nr-point.h>
#include <libnr/nr-point-fns.h>
#include <libnr/nr-matrix.h>
#include <libnr/nr-rotate.h>
#include <libnr/nr-rotate-ops.h>
#include <math.h>

/*
 * stroking polylines into a Shape instance
 * grunt work.
 * if the goal is to raster the stroke, polyline stroke->polygon->uncrossed polygon->raster is grossly
 * inefficient (but reuse the intersector, so that's what a lazy programmer like me does). the correct way would be
 * to set up a supersampled buffer, raster each polyline stroke's part (one part per segment in the polyline, plus 
 * each join) because these are all convex polygons, then transform in alpha values
 */


// to make round caps and joins really arcs
// otherwise it's approximated with a (faster) quadratic bezier
//#define joli_recround

// until i find something better
NR::Point StrokeNormalize(const NR::Point v) {
	double l = L2(v); 
    if ( l < 0.0000001 ) { 
		return NR::Point(0, 0);
    } else { 
		return v/l; 
    }
}

void Path::Stroke(Shape *dest, bool doClose, double width, JoinType join,
                  ButtType butt, double miter, bool justAdd)
{
    if (dest == NULL) {
        return;
    }
    
    if (justAdd == false) {
        dest->Reset(3 * pts.size(), 3 * pts.size());
    }
    
    dest->MakeBackData(false);

    int lastM = 0;
    while (lastM < int(pts.size())) {

        int lastP = lastM + 1;
        while (lastP < int(pts.size()) // select one subpath
               && (pts[lastP].isMoveTo == polyline_lineto
                   || pts[lastP].isMoveTo == polyline_forced))
        {
	    lastP++;
        }

        if ( lastP > lastM+1 ) {
            NR::Point sbStart = pts[lastM].p;
            NR::Point sbEnd = pts[lastP - 1].p;
            if ( NR::LInfty(sbEnd-sbStart) < 0.00001 ) {       // why close lines that shouldn't be closed? ah I see, because close is defined here for a hole path and should be defined per subpath.
                // debut==fin => ferme (on devrait garder un element pour les close(), mais tant pis)
                DoStroke(lastM, lastP - lastM, dest, true, width, join, butt, miter, true);
            } else {
                DoStroke(lastM, lastP - lastM, dest, doClose, width, join, butt, miter, true);
            }
        } else if (butt == butt_round) {       // special case: zero length round butt is a circle
            int last[2] = { -1, -1 };
            NR::Point dir;
            dir[0] = 1;
            dir[1] = 0;
            NR::Point pos = pts[lastM].p;
            DoButt(dest, width, butt, pos, dir, last[RIGHT], last[LEFT]);
            int end[2];
            dir = -dir;
            DoButt(dest, width, butt, pos, dir, end[LEFT], end[RIGHT]);
            dest->AddEdge (end[LEFT], last[LEFT]);
            dest->AddEdge (last[RIGHT], end[RIGHT]);
        }
        lastM = lastP;
    }
}

void Path::DoStroke(int off, int N, Shape *dest, bool doClose, double width, JoinType join,
                    ButtType butt, double miter, bool justAdd)
{
    if (N <= 1) {
        return;
    }
  
    NR::Point prevP, nextP;
    int prevI, nextI;
    int upTo;
  
    int curI = 0;
    NR::Point curP = pts[off].p;
  
    if (doClose) {

        prevI = N - 1;
        while (prevI > 0) {
            prevP = pts[off + prevI].p;
            NR::Point diff = curP - prevP;
            double dist = dot(diff, diff);
            if (dist > 0.001) {
                break;
            }
            prevI--;
        }
        if (prevI <= 0) {
            return;
        }
        upTo = prevI;

    } else {
        
        prevP = curP;
        prevI = curI;
        upTo = N - 1;
    }
    
    {
        nextI = 1;
        while (nextI <= upTo) {
            nextP = pts[off + nextI].p;
            NR::Point diff = curP - nextP;
            double dist = dot(diff, diff);
            if (dist > 0.001) {
                break;
            }
            nextI++;
        }
        if (nextI > upTo) {
            if (butt == butt_round) {  // special case: (nearly) zero length round butt is a circle
                int last[2] = { -1, -1 };
                NR::Point dir;
                dir[0] = 1;
                dir[1] = 0;
                DoButt(dest, width, butt, curP, dir, last[RIGHT], last[LEFT]);
                int end[2];
                dir = -dir;
                DoButt(dest, width, butt, curP, dir, end[LEFT], end[RIGHT]);
                dest->AddEdge (end[LEFT], last[LEFT]);
                dest->AddEdge (last[RIGHT], end[RIGHT]);
            }
            return;
        }
    }

    int start[2] = { -1, -1 };
    int last[2] = { -1, -1 };
    NR::Point prevD = curP - prevP;
    NR::Point nextD = nextP - curP;
    double prevLe = NR::L2(prevD);
    double nextLe = NR::L2(nextD);
    prevD = StrokeNormalize(prevD);
    nextD = StrokeNormalize(nextD);
    
    if (doClose) {
        DoJoin(dest,  width, join, curP, prevD, nextD, miter, prevLe, nextLe, start, last);
    } else {
        nextD = -nextD;
        DoButt(dest,  width, butt, curP, nextD, last[RIGHT], last[LEFT]);
        nextD = -nextD;
    }
    
    do {
        prevP = curP;
        prevI = curI;
        curP = nextP;
        curI = nextI;
        prevD = nextD;
        prevLe = nextLe;
        nextI++;
        while (nextI <= upTo) {
            nextP = pts[off + nextI].p;
            NR::Point diff = curP - nextP;
            double dist = dot(diff, diff);
            if (dist > 0.001) {
                break;
            }
            nextI++;
        }
        if (nextI > upTo) {
            break;
        }
    
        nextD = nextP - curP;
        nextLe = NR::L2(nextD);
        nextD = StrokeNormalize(nextD);
        int nSt[2] = { -1, -1 };
        int nEn[2] = { -1, -1 };
        DoJoin(dest, width, join, curP, prevD, nextD, miter, prevLe, nextLe, nSt, nEn);
        dest->AddEdge(nSt[LEFT], last[LEFT]);
        last[LEFT] = nEn[LEFT];
        dest->AddEdge(last[RIGHT], nSt[RIGHT]);
        last[RIGHT] = nEn[RIGHT];
    } while (nextI <= upTo);
    
    if (doClose) {
		/*		prevP=curP;
				prevI=curI;
				curP=nextP;
				curI=nextI;
				prevD=nextD;*/
        nextP = pts[off].p;

        nextD = nextP - curP;
        nextLe = NR::L2(nextD);
        nextD = StrokeNormalize(nextD);
        int nSt[2] = { -1, -1 };
        int nEn[2] = { -1, -1 };
        DoJoin(dest,  width, join, curP, prevD, nextD, miter, prevLe, nextLe, nSt, nEn);
        dest->AddEdge (nSt[LEFT], last[LEFT]);
        last[LEFT] = nEn[LEFT];
        dest->AddEdge (last[RIGHT], nSt[RIGHT]);
        last[RIGHT] = nEn[RIGHT];
        
        dest->AddEdge (start[LEFT], last[LEFT]);
        dest->AddEdge (last[RIGHT], start[RIGHT]);
        
    } else {

        int end[2];
        DoButt (dest,  width, butt, curP, prevD, end[LEFT], end[RIGHT]);
        dest->AddEdge (end[LEFT], last[LEFT]);
        dest->AddEdge (last[RIGHT], end[RIGHT]);
    }
}


void Path::DoButt(Shape *dest, double width, ButtType butt, NR::Point pos, NR::Point dir,
                  int &leftNo, int &rightNo)
{
	NR::Point nor;
	nor = dir.ccw();
  
	if (butt == butt_square)
	{
		NR::Point x;
		x = pos + width * dir + width * nor;
		int bleftNo = dest->AddPoint (x);
		x = pos + width * dir - width * nor;
		int brightNo = dest->AddPoint (x);
		x = pos + width * nor;
		leftNo = dest->AddPoint (x);
		x = pos - width * nor;
		rightNo = dest->AddPoint (x);
		dest->AddEdge (rightNo, brightNo);
		dest->AddEdge (brightNo, bleftNo);
		dest->AddEdge (bleftNo, leftNo);
	}
	else if (butt == butt_pointy)
	{
		leftNo = dest->AddPoint (pos + width * nor);
		rightNo = dest->AddPoint (pos - width * nor);
		int mid = dest->AddPoint (pos + width * dir);
		dest->AddEdge (rightNo, mid);
		dest->AddEdge (mid, leftNo);
	}
	else if (butt == butt_round)
	{
		const NR::Point sx = pos + width * nor;
		const NR::Point ex = pos - width * nor;
		const NR::Point mx = pos + width * dir;
		leftNo = dest->AddPoint (sx);
		rightNo = dest->AddPoint (ex);
		int midNo = dest->AddPoint (mx);
    
		NR::Point dx = pos - width * nor + width * dir;
		RecRound (dest, rightNo, midNo, dx, ex, mx, 5.0, 8,pos,width);
		dx = pos + width * nor + width * dir;
		RecRound (dest, midNo, leftNo, dx, mx, sx, 5.0, 8,pos,width);
	}
	else
	{
		leftNo = dest->AddPoint (pos + width * nor);
		rightNo = dest->AddPoint (pos - width * nor);
		dest->AddEdge (rightNo, leftNo);
	}
}


void Path::DoJoin (Shape *dest, double width, JoinType join, NR::Point pos, NR::Point prev,
                   NR::Point next, double miter, double prevL, double nextL,
                   int *stNo, int *enNo)
{
    NR::Point pnor = prev.ccw();
    NR::Point nnor = next.ccw();
    double angSi = cross(next, prev);

    /* FIXME: this special case causes bug 1028953 */
    if (angSi > -0.0001 && angSi < 0.0001) {
        double angCo = dot (prev, next);
        if (angCo > 0.9999) {
            // tout droit
            stNo[LEFT] = enNo[LEFT] = dest->AddPoint(pos + width * pnor);
            stNo[RIGHT] = enNo[RIGHT] = dest->AddPoint(pos - width * pnor);
        } else {
            // demi-tour
            stNo[LEFT] = enNo[RIGHT] = dest->AddPoint (pos + width * pnor);
            stNo[RIGHT] = enNo[LEFT] = dest->AddPoint (pos - width * pnor);
            dest->AddEdge(enNo[LEFT], stNo[LEFT]);
            dest->AddEdge(stNo[RIGHT], enNo[RIGHT]);
        }
        return;
    }
    
    if (angSi < 0) {
        int midNo = dest->AddPoint(pos);
        stNo[LEFT] = dest->AddPoint(pos + width * pnor);
        enNo[LEFT] = dest->AddPoint(pos + width * nnor);
        dest->AddEdge(enNo[LEFT], midNo);
        dest->AddEdge(midNo, stNo[LEFT]);
        
        if (join == join_pointy) {

            stNo[RIGHT] = dest->AddPoint(pos - width * pnor);
            enNo[RIGHT] = dest->AddPoint(pos - width * nnor);
      
            const NR::Point biss = StrokeNormalize(prev - next);
            double c2 = dot(biss, nnor);
            double l = width / c2;
            double emiter = width * c2;
            if (emiter < miter) {
                emiter = miter;
            }
            
            if (fabs(l) < miter) {
                int const n = dest->AddPoint(pos - l * biss);
                dest->AddEdge(stNo[RIGHT], n);
                dest->AddEdge(n, enNo[RIGHT]);
            } else {
                dest->AddEdge(stNo[RIGHT], enNo[RIGHT]);
            }
            
        } else if (join == join_round) {

            NR::Point sx = pos - width * pnor;
            stNo[RIGHT] = dest->AddPoint(sx);
            NR::Point ex = pos - width * nnor;
            enNo[RIGHT] = dest->AddPoint(ex);
      
            const NR::Point biss = StrokeNormalize(pnor + nnor);
            double c2 = dot(biss, nnor);
            double l = width / c2;
            double typ = dot(pnor, nnor);
            if (typ >= 0) {
                RecRound(dest, stNo[RIGHT], enNo[RIGHT], pos - l * biss, 
                         sx, ex, 5.0, 8,pos,width);
            } else {
                double s2 = cross(biss, nnor);
                double dec = (l - width) * c2 / s2;
                NR::Point tbiss = biss.cw();
        
                NR::Point nsx = pos - width * biss - dec * tbiss;
                NR::Point nex = pos - width * biss + dec * tbiss;
                NR::Point mx = pos - width * biss;
                int midNo = dest->AddPoint(mx);
                RecRound(dest, stNo[RIGHT], midNo, nsx, sx, mx, 5.0, 8, pos, width);
                RecRound(dest, midNo, enNo[RIGHT], nex, mx, ex, 5.0, 8, pos, width);
            }
            
        } else {
            
            stNo[RIGHT] = dest->AddPoint(pos - width * pnor);
            enNo[RIGHT] = dest->AddPoint(pos - width * nnor);
            dest->AddEdge(stNo[RIGHT], enNo[RIGHT]);
        }
        
    } else {
        
        int midNo = dest->AddPoint(pos);
        stNo[RIGHT] = dest->AddPoint(pos - width * pnor);
        enNo[RIGHT] = dest->AddPoint(pos - width * nnor);
        dest->AddEdge(stNo[RIGHT], midNo);
        dest->AddEdge(midNo, enNo[RIGHT]);
        
        if (join == join_pointy) {
            
            stNo[LEFT] = dest->AddPoint(pos + width * pnor);
            enNo[LEFT] = dest->AddPoint(pos + width * nnor);
      
            const NR::Point biss = StrokeNormalize(next - prev);
            double c2 = dot(biss, nnor);
            double l = width / c2;
            double emiter = width * c2;
            if (emiter < miter) {
                emiter = miter;
            }
            if ( fabs(l) < miter) {
                int const n = dest->AddPoint (pos + l * biss);
                dest->AddEdge (enNo[LEFT], n);
                dest->AddEdge (n, stNo[LEFT]);
            }
            else
            {
                dest->AddEdge (enNo[LEFT], stNo[LEFT]);
            }
            
        } else if (join == join_round) {

            NR::Point sx = pos + width * pnor;
            stNo[LEFT] = dest->AddPoint(sx);
            NR::Point ex = pos + width * nnor;
            enNo[LEFT] = dest->AddPoint(ex);
      
            const NR::Point biss = StrokeNormalize(pnor + nnor);
            double c2 = dot(biss, nnor);
            double l = width / c2;
            double typ = dot(pnor, nnor);
            if (typ >= 0) {
                RecRound (dest, enNo[LEFT], stNo[LEFT], pos + l * biss, ex, sx, 5.0, 8, pos, width);
            } else {
                double s2 = cross (biss, nnor);
                double dec = (l - width) * c2 / s2;
                NR::Point tbiss = biss.cw();
                
                NR::Point nsx = pos + width * biss + dec * tbiss;
                NR::Point nex = pos + width * biss - dec * tbiss;
                NR::Point mx = pos + width * biss;
                int midNo = dest->AddPoint (mx);
                RecRound(dest, enNo[LEFT], midNo, nex, ex, mx, 5.0, 8, pos, width);
                RecRound(dest, midNo, stNo[LEFT], nsx, mx, sx, 5.0, 8, pos, width);
            }
            
        } else {
            
            stNo[LEFT] = dest->AddPoint(pos + width * pnor);
            enNo[LEFT] = dest->AddPoint(pos + width * nnor);
            dest->AddEdge(enNo[LEFT], stNo[LEFT]);
            
        }
    }
}

void
Path::DoLeftJoin (Shape * dest, double width, JoinType join, NR::Point pos,
                  NR::Point prev, NR::Point next, double miter, double prevL, double nextL,
                  int &leftStNo, int &leftEnNo,int pathID,int pieceID,double tID)
{
	NR::Point pnor=prev.ccw();
	NR::Point nnor=next.ccw();
	double angSi = cross (next, prev);
	if (angSi > -0.0001 && angSi < 0.0001)
	{
		double angCo = dot (prev, next);
		if (angCo > 0.9999)
		{
			// tout droit
			leftEnNo = leftStNo = dest->AddPoint (pos + width * pnor);
		}
		else
		{
			// demi-tour
			leftStNo = dest->AddPoint (pos + width * pnor);
			leftEnNo = dest->AddPoint (pos - width * pnor);
			int nEdge=dest->AddEdge (leftEnNo, leftStNo);
			if ( dest->hasBackData() ) {
				dest->ebData[nEdge].pathID=pathID;
				dest->ebData[nEdge].pieceID=pieceID;
				dest->ebData[nEdge].tSt=dest->ebData[nEdge].tEn=tID;
			}
		}
		return;
	}
	if (angSi < 0)
	{
		/*		NR::Point     biss;
				biss.x=next.x-prev.x;
				biss.y=next.y-prev.y;
				double   c2=cross(biss,next);
				double   l=width/c2;
				double		projn=l*(dot(biss,next));
				double		projp=-l*(dot(biss,prev));
				if ( projp <= 0.5*prevL && projn <= 0.5*nextL ) {
				double   x,y;
				x=pos.x+l*biss.x;
				y=pos.y+l*biss.y;
				leftEnNo=leftStNo=dest->AddPoint(x,y);
				} else {*/
		leftStNo = dest->AddPoint (pos + width * pnor);
		leftEnNo = dest->AddPoint (pos + width * nnor);
		int midNo = dest->AddPoint (pos);
		int nEdge=dest->AddEdge (leftEnNo, midNo);
		if ( dest->hasBackData() ) {
			dest->ebData[nEdge].pathID=pathID;
			dest->ebData[nEdge].pieceID=pieceID;
			dest->ebData[nEdge].tSt=dest->ebData[nEdge].tEn=tID;
		}
		nEdge=dest->AddEdge (midNo, leftStNo);
		if ( dest->hasBackData() ) {
			dest->ebData[nEdge].pathID=pathID;
			dest->ebData[nEdge].pieceID=pieceID;
			dest->ebData[nEdge].tSt=dest->ebData[nEdge].tEn=tID;
		}
		//              }
	}
	else
	{
		if (join == join_pointy)
		{
			leftStNo = dest->AddPoint (pos + width * pnor);
			leftEnNo = dest->AddPoint (pos + width * nnor);
      
			const NR::Point biss = StrokeNormalize (pnor + nnor);
			double c2 = dot (biss, nnor);
			double l = width / c2;
			double emiter = width * c2;
			if (emiter < miter)
				emiter = miter;
			if (l <= emiter)
			{
				int nleftStNo = dest->AddPoint (pos + l * biss);
				int nEdge=dest->AddEdge (leftEnNo, nleftStNo);
				if ( dest->hasBackData() ) {
					dest->ebData[nEdge].pathID=pathID;
					dest->ebData[nEdge].pieceID=pieceID;
					dest->ebData[nEdge].tSt=dest->ebData[nEdge].tEn=tID;
				}
				nEdge=dest->AddEdge (nleftStNo, leftStNo);
				if ( dest->hasBackData() ) {
					dest->ebData[nEdge].pathID=pathID;
					dest->ebData[nEdge].pieceID=pieceID;
					dest->ebData[nEdge].tSt=dest->ebData[nEdge].tEn=tID;
				}
			}
			else
			{
				double s2 = cross (biss, nnor);
				double dec = (l - emiter) * c2 / s2;
				const NR::Point tbiss=biss.ccw();
        
				int nleftStNo = dest->AddPoint (pos + emiter * biss + dec * tbiss);
				int nleftEnNo = dest->AddPoint (pos + emiter * biss - dec * tbiss);
				int nEdge=dest->AddEdge (nleftEnNo, nleftStNo);
				if ( dest->hasBackData() ) {
					dest->ebData[nEdge].pathID=pathID;
					dest->ebData[nEdge].pieceID=pieceID;
					dest->ebData[nEdge].tSt=dest->ebData[nEdge].tEn=tID;
				}
				nEdge=dest->AddEdge (leftEnNo, nleftEnNo);
				if ( dest->hasBackData() ) {
					dest->ebData[nEdge].pathID=pathID;
					dest->ebData[nEdge].pieceID=pieceID;
					dest->ebData[nEdge].tSt=dest->ebData[nEdge].tEn=tID;
				}
				nEdge=dest->AddEdge (nleftStNo, leftStNo);
				if ( dest->hasBackData() ) {
					dest->ebData[nEdge].pathID=pathID;
					dest->ebData[nEdge].pieceID=pieceID;
					dest->ebData[nEdge].tSt=dest->ebData[nEdge].tEn=tID;
				}
			}
		}
		else if (join == join_round)
		{
			const NR::Point sx = pos + width * pnor;
			leftStNo = dest->AddPoint (sx);
			const NR::Point ex = pos + width * nnor;
			leftEnNo = dest->AddPoint (ex);
      
			const NR::Point biss = StrokeNormalize (pnor + nnor);
			const double c2 = dot (biss, nnor);
			const double l = width / c2;
			const double typ = dot (pnor, nnor);
			if (typ >= 0)
			{
				RecRound (dest, leftEnNo, leftStNo, pos + l * biss, ex, sx, 5.0,
						  8,pos,width);
			}
			else
			{
				double s2 = cross (biss, nnor);
				double dec = (l - width) * c2 / s2;
				NR::Point tbiss=biss.cw();
        
				NR::Point nsx = pos + width * biss + dec * tbiss;
				NR::Point nex = pos + width * biss - dec * tbiss;
				NR::Point mx = pos + width * biss;
				const int midNo = dest->AddPoint (mx);
        RecRound (dest, leftEnNo, midNo, nex, ex, mx, 5.0,
						  8,pos,width);
				RecRound (dest, midNo, leftStNo, nsx, mx, sx, 5.0,
						  8,pos,width);
			}
		}
		else
		{
			leftStNo = dest->AddPoint (pos + width * pnor);
			leftEnNo = dest->AddPoint (pos + width * nnor);
			int nEdge=dest->AddEdge (leftEnNo, leftStNo);
			if ( dest->hasBackData() ) {
				dest->ebData[nEdge].pathID=pathID;
				dest->ebData[nEdge].pieceID=pieceID;
				dest->ebData[nEdge].tSt=dest->ebData[nEdge].tEn=tID;
			}
		}
	}
}
void
Path::DoRightJoin (Shape * dest, double width, JoinType join, NR::Point pos,
                   NR::Point prev, NR::Point next, double miter, double prevL,
                   double nextL, int &rightStNo, int &rightEnNo,int pathID,int pieceID,double tID)
{
	const NR::Point pnor=prev.ccw();
	const NR::Point nnor=next.ccw();
	double angSi = cross (next,prev);
	if (angSi > -0.0001 && angSi < 0.0001)
	{
		double angCo = dot (prev, next);
		if (angCo > 0.9999)
		{
			// tout droit
			rightEnNo = rightStNo = dest->AddPoint (pos - width*pnor);
		}
		else
		{
			// demi-tour
			rightEnNo = dest->AddPoint (pos + width*pnor);
			rightStNo = dest->AddPoint (pos - width*pnor);
			int nEdge=dest->AddEdge (rightStNo, rightEnNo);
			if ( dest->hasBackData() ) {
				dest->ebData[nEdge].pathID=pathID;
				dest->ebData[nEdge].pieceID=pieceID;
				dest->ebData[nEdge].tSt=dest->ebData[nEdge].tEn=tID;
			}
		}
		return;
	}
	if (angSi < 0)
	{
		if (join == join_pointy)
		{
			rightStNo = dest->AddPoint (pos - width*pnor);
			rightEnNo = dest->AddPoint (pos - width*nnor);
      
			const NR::Point biss = StrokeNormalize (pnor + nnor);
			double c2 = dot (biss, nnor);
			double l = width / c2;
			double emiter = width * c2;
			if (emiter < miter)
				emiter = miter;
			if (l <= emiter)
			{
				int nrightStNo = dest->AddPoint (pos - l * biss);
				int nEdge=dest->AddEdge (rightStNo, nrightStNo);
				if ( dest->hasBackData() ) {
					dest->ebData[nEdge].pathID=pathID;
					dest->ebData[nEdge].pieceID=pieceID;
					dest->ebData[nEdge].tSt=dest->ebData[nEdge].tEn=tID;
				}
				nEdge=dest->AddEdge (nrightStNo, rightEnNo);
				if ( dest->hasBackData() ) {
					dest->ebData[nEdge].pathID=pathID;
					dest->ebData[nEdge].pieceID=pieceID;
					dest->ebData[nEdge].tSt=dest->ebData[nEdge].tEn=tID;
				}
			}
			else
			{
				double s2 = cross (biss, nnor);
				double dec = (l - emiter) * c2 / s2;
				const NR::Point tbiss=biss.ccw();
        
				int nrightStNo = dest->AddPoint (pos - emiter*biss - dec*tbiss);
				int nrightEnNo = dest->AddPoint (pos - emiter*biss + dec*tbiss);
				int nEdge=dest->AddEdge (rightStNo, nrightStNo);
				if ( dest->hasBackData() ) {
					dest->ebData[nEdge].pathID=pathID;
					dest->ebData[nEdge].pieceID=pieceID;
					dest->ebData[nEdge].tSt=dest->ebData[nEdge].tEn=tID;
				}
				nEdge=dest->AddEdge (nrightStNo, nrightEnNo);
				if ( dest->hasBackData() ) {
					dest->ebData[nEdge].pathID=pathID;
					dest->ebData[nEdge].pieceID=pieceID;
					dest->ebData[nEdge].tSt=dest->ebData[nEdge].tEn=tID;
				}
				nEdge=dest->AddEdge (nrightEnNo, rightEnNo);
				if ( dest->hasBackData() ) {
					dest->ebData[nEdge].pathID=pathID;
					dest->ebData[nEdge].pieceID=pieceID;
					dest->ebData[nEdge].tSt=dest->ebData[nEdge].tEn=tID;
				}
			}
		}
		else if (join == join_round)
		{
			const NR::Point sx = pos - width * pnor;
			rightStNo = dest->AddPoint (sx);
			const NR::Point ex = pos - width * nnor;
			rightEnNo = dest->AddPoint (ex);
      
			const NR::Point biss = StrokeNormalize (pnor + nnor);
			double c2 = dot (biss, nnor);
			double l = width / c2;
			double typ = dot (pnor, nnor);
			if (typ >= 0)
			{
				RecRound (dest, rightStNo, rightEnNo, pos - l*biss, 
						  sx, ex, 5.0, 8,pos,width);
			}
			else
			{
				double s2 = cross (biss, nnor);
				double dec = (l - width) * c2 / s2;
				NR::Point tbiss=biss.cw();
        
				NR::Point nsx = pos - width * biss - dec * tbiss;
				NR::Point nex = pos - width * biss + dec * tbiss;
				NR::Point mx = pos - width * biss;
				int midNo = dest->AddPoint (mx);
				RecRound (dest, rightStNo, midNo, nsx, sx, mx, 5.0,
						  8,pos,width);
				RecRound (dest, midNo, rightEnNo, nex, mx, ex, 5.0,
						  8,pos,width);
			}
		}
		else
		{
			rightStNo = dest->AddPoint (pos - width * pnor);
			rightEnNo = dest->AddPoint (pos - width * nnor);
			int nEdge=dest->AddEdge (rightStNo, rightEnNo);
			if ( dest->hasBackData() ) {
				dest->ebData[nEdge].pathID=pathID;
				dest->ebData[nEdge].pieceID=pieceID;
				dest->ebData[nEdge].tSt=dest->ebData[nEdge].tEn=tID;
			}
		}
	}
	else
	{
		/*		NR::Point     biss;
				biss=next.x-prev.x;
				biss.y=next.y-prev.y;
				double   c2=cross(next,biss);
				double   l=width/c2;
				double		projn=l*(dot(biss,next));
				double		projp=-l*(dot(biss,prev));
				if ( projp <= 0.5*prevL && projn <= 0.5*nextL ) {
				double   x,y;
				x=pos.x+l*biss.x;
				y=pos.y+l*biss.y;
				rightEnNo=rightStNo=dest->AddPoint(x,y);
				} else {*/
		rightStNo = dest->AddPoint (pos - width*pnor);
		rightEnNo = dest->AddPoint (pos - width*nnor);
		int midNo = dest->AddPoint (pos);
		int nEdge=dest->AddEdge (rightStNo, midNo);
		if ( dest->hasBackData() ) {
			dest->ebData[nEdge].pathID=pathID;
			dest->ebData[nEdge].pieceID=pieceID;
			dest->ebData[nEdge].tSt=dest->ebData[nEdge].tEn=tID;
		}
		nEdge=dest->AddEdge (midNo, rightEnNo);
		if ( dest->hasBackData() ) {
			dest->ebData[nEdge].pathID=pathID;
			dest->ebData[nEdge].pieceID=pieceID;
			dest->ebData[nEdge].tSt=dest->ebData[nEdge].tEn=tID;
		}
		//              }
	}
}


// a very ugly way to produce round joins: doing one (or two, depend on the angle of the join) quadratic bezier curves
// but since most joins are going to be small, nobody will notice
void Path::RecRound(Shape *dest, int sNo, int eNo,
                    NR::Point const &iP, NR::Point const &iS, NR::Point const &iE,
                    double tresh, int lev, NR::Point &origine, float width)
{
#ifdef joli_recround
    NR::Point stN = iS - origine;
    NR::Point enN = iE - origine;
    stN = StrokeNormalize(stN);
    enN = StrokeNormalize(enN);
    double coa = dot(stN, enN);
    double sia = cross(stN, enN);
    double ang = acos(coa);
    if ( coa >= 1 ) {
        ang = 0;
    }
    if ( coa <= -1 ) {
        ang = M_PI;
    }
    ang /= 0.1;
    
    int nbS = (int) floor(ang);
    NR::rotate omega(((sia > 0) ? -0.1 : 0.1));
    NR::Point cur = iS - origine;
//  StrokeNormalize(cur);
//  cur*=width;
    int lastNo = sNo;
    for (int i = 1; i < nbS; i++) {
        cur = cur * omega;
        NR::Point m = origine + cur;
        int mNo = dest->AddPoint(m);
        dest->AddEdge(lastNo, mNo);
        lastNo = mNo;
    }
    dest->AddEdge(lastNo, eNo);
#else
    if (lev <= 0) {
        dest->AddEdge (sNo, eNo);
        return;
    }
    
    const double s = fabs(cross(iE - iP,iS - iP));
    if (s < tresh) {
        dest->AddEdge(sNo, eNo);
        return;
    }
  
    const NR::Point m = (iS + iE + 2 * iP) / 4;
    int mNo = dest->AddPoint (m);
  
    RecRound(dest, sNo, mNo, (iS + iP) / 2, iS, m, tresh, lev - 1, origine, width);
    RecRound(dest, mNo, eNo, (iE + iP) / 2, m, iE, tresh, lev - 1, origine, width);
#endif
}

/*
 * dashed version
 * nota: dashes produced this way are a bot different from the usual ones, because cap styles are not
 * applied to the dashes
 */

void Path::Stroke(Shape *dest, bool doClose, double width, JoinType join,
                  ButtType butt, double miter, int nbDash, one_dash *dashs,
                  bool justAdd)
{
    if (nbDash <= 0) {
        Stroke(dest, doClose, width, join, butt, miter, justAdd);
        return;
    }
  
    if (dest == NULL) {
        return;
    }
    
    if (justAdd == false) {
        dest->Reset(3 * pts.size(), 3 * pts.size());
    }
    
    if (pts.size() <= 1) {
        return;
    }
    
    dest->MakeBackData(false);

    int lastM = 0;
    while (lastM < int(pts.size())) {
        int lastP = lastM + 1;
        while (lastP < int(pts.size())
               && (pts[lastP].isMoveTo == polyline_lineto
                   || pts[lastP].isMoveTo == polyline_forced))
        {
            lastP++;
        }

        DoStroke(lastM, lastP - lastM, dest, doClose, width, join, butt, miter, nbDash, dashs, true);
        lastM = lastP;
    }
}

void Path::DoStroke(int off, int N, Shape *dest, bool doClose, double width, JoinType join,
                    ButtType butt, double miter, int nbDash, one_dash *dashs,
                    bool justAdd)
{
    if (dest == NULL) {
        return;
    }
    
    if (N <= 1) {
        return;
    }
  
    NR::Point prevP;
    NR::Point nextP;
    int prevI;
    int nextI;
    int upTo;
    double curA;
    double prevA;
    double nextA;
    double dashPos = 0;
    double dashAbs = 0;
    int dashNo = 0;
    
    int curI = 0;
    NR::Point curP = pts[off].p;
  
    if (doClose) {

        prevI = N - 1;
        while (prevI > 0) {
            prevP = pts[off + prevI].p;
            NR::Point diff = curP - prevP;
            double dist = dot(diff, diff);
            if (dist > 0.001) {
                break;
            }
            prevI--;
        }
        if (prevI <= 0) {
            return;
        }
        upTo = prevI;
        
    } else {
        prevP = curP;
        prevI = curI;
        upTo = N - 1;
    }
    
    {
        nextI = 1;
        while (nextI <= upTo) {
            nextP = pts[off + nextI].p;
            NR::Point diff = curP - nextP;
            double dist = dot(diff, diff);
            if (dist > 0.001) {
                break;
            }
            
            nextI++;
        }
        if (nextI > upTo) {
            return;
        }
    }

    int start[2] = { -1, -1 };
    int last[2] = { -1, -1 };
    bool noStartJoin = false, inGap = true;
    NR::Point prevD, nextD;
    double prevLe, nextLe;
    prevD = curP - prevP;
    nextD = nextP - curP;
    curA = 0;
    prevA = -NR::L2(prevD);
    nextA = NR::L2(nextD);
    prevLe = NR::L2(prevD);
    nextLe = NR::L2(nextD);
    prevD = StrokeNormalize(prevD);
    nextD = StrokeNormalize(nextD);
    dashTo_info dTo;
    
    {
        int cDashNo = dashNo;
        double nDashAbs = 0;
        do {
            if (dashAbs + (dashs[dashNo].length - dashPos) <= nDashAbs) {

                dashNo++;
                if (dashNo >= nbDash) {
                    dashNo -= nbDash;
                }
                dashPos = 0;
                
            } else {
                break;
            }
        } while (dashNo != cDashNo);
        
    }
    
    if (doClose) {

        if (dashs[dashNo].gap) {

            noStartJoin = true;
            inGap = true;
            
        } else {
            
            noStartJoin = false;
            DoJoin(dest, width, join, curP, prevD, nextD, miter, prevLe, nextLe, start, last);
            inGap = false;
        }
        
    } else {

        if (dashs[dashNo].gap) {
            inGap = true;
        } else {
            nextD = -nextD;
            DoButt(dest,  width, butt, curP, nextD, last[RIGHT], last[LEFT]);
            nextD = -nextD;
            inGap = false;
        }
    }
    
    do {
        prevP = curP;
        prevI = curI;
        prevA = curA;
        curP = nextP;
        curI = nextI;
        curA = nextA;
        prevLe = nextLe;
        prevD = nextD;
        nextI++;
        while (nextI <= upTo) {
            nextP = pts[off + nextI].p;
            NR::Point diff = curP - nextP;
            double dist = dot(diff, diff);
            if (dist > 0.001) {
                break;
            }
            
            nextI++;
        }
        
        if (nextI > upTo) {
            break;
        }
    
        nextD = nextP - curP;
        nextA = curA + NR::L2(nextD);
        nextLe = NR::L2(nextD);
        nextD = StrokeNormalize(nextD);
    
        dTo.nDashAbs = curA;
        dTo.prevP = prevP;
        dTo.curP = curP;
        dTo.prevD = prevD;
        dTo.prevW = width;
        dTo.curW = width;
    
        DashTo(dest, &dTo, dashAbs, dashNo, dashPos, inGap, last[LEFT],
               last[RIGHT], nbDash, dashs);
    
        if (inGap == false) {
            int nSt[2] = { -1, -1 };
            int nEn[2] = { -1, -1 };
            DoJoin(dest, width, join, curP, prevD, nextD, miter, prevLe, nextLe, nSt, nEn);
            
            dest->AddEdge (nSt[LEFT], last[LEFT]);
            last[LEFT] = nEn[LEFT];
            dest->AddEdge (last[RIGHT], nSt[RIGHT]);
            last[RIGHT] = nEn[RIGHT];
        }
    } while (nextI <= upTo);
    
    if (doClose) {
        /*		prevP=curP;
                        prevI=curI;
                        prevA=curA;
                        curP=nextP;
                        curI=nextI;
                        curA=nextA;
                        prevD=nextD;*/
        
        nextP = pts[off].p;
    
        nextD = nextP - curP;
        nextA = curA + L2(nextD);
        nextLe = NR::L2(nextD);
        nextD = StrokeNormalize(nextD);
    
        dTo.nDashAbs = curA;
        dTo.prevP = prevP;
        dTo.curP = curP;
        dTo.prevD = prevD;
        dTo.prevW = width;
        dTo.curW =  width;
        
        DashTo(dest, &dTo, dashAbs, dashNo, dashPos, inGap, last[LEFT],
               last[RIGHT], nbDash, dashs);
        
        if (inGap == false) {
            int nSt[2] = { -1, -1 };
            int nEn[2] = { -1, -1 };
            DoJoin(dest,  width, join, curP, prevD, nextD, miter, prevLe, nextLe, nSt, nEn);
            dest->AddEdge (nSt[LEFT], last[LEFT]);
            last[LEFT] = nEn[LEFT];
            dest->AddEdge (last[RIGHT], nSt[RIGHT]);
            last[RIGHT] = nEn[RIGHT];
        }
        
        dTo.nDashAbs = nextA;
        dTo.prevP = curP;
        dTo.curP = nextP;
        dTo.prevD = nextD;
        dTo.prevW = width;
        dTo.curW = width;
    
        DashTo(dest, &dTo, dashAbs, dashNo, dashPos, inGap, last[LEFT],
               last[RIGHT], nbDash, dashs);
    
        if (inGap == false) {
            if (noStartJoin == false) {
                dest->AddEdge(start[LEFT], last[LEFT]);
                dest->AddEdge(last[RIGHT], start[RIGHT]);
            } else {
                dest->AddEdge(last[RIGHT], last[LEFT]);
            }
        } else {
            if (noStartJoin == false) {
                dest->AddEdge(start[LEFT], start[RIGHT]);
            }
        }
        
    } else {
        
        dTo.nDashAbs = curA;
        dTo.prevP = prevP;
        dTo.curP = curP;
        dTo.prevD = prevD;
        dTo.prevW = width;
        dTo.curW = width;
        
        DashTo(dest, &dTo, dashAbs, dashNo, dashPos, inGap, last[LEFT],
               last[RIGHT], nbDash, dashs);
        
        if (inGap == false) {
            int end[2];
            DoButt(dest,  width, butt, curP, prevD, end[LEFT], end[RIGHT]);
            dest->AddEdge(end[LEFT], last[LEFT]);
            dest->AddEdge(last[RIGHT], end[RIGHT]);
        }
    }
}

void Path::DashTo(Shape *dest, dashTo_info *dTo, double &dashAbs, int &dashNo,
                  double &dashPos, bool &inGap, int &lastLeft, int &lastRight,
                  int nbDash, one_dash *dashs)
{
    //      printf("%f %i %f %i -> %f\n",dashAbs,dashNo,dashPos,(inGap)?1:0,dTo->nDashAbs);
    NR::Point pnor = dTo->prevD.ccw();
  
    double oDashAbs = dashAbs;
    while (dashAbs < dTo->nDashAbs) {
        int cDashNo = dashNo;
        do {
            double delta = dashs[dashNo].length - dashPos;
            if (delta <= dTo->nDashAbs - dashAbs) {
                dashNo++;
                dashPos = 0;
                if (dashNo >= nbDash) {
                    dashNo -= nbDash;
                }
                
                while (dashNo != cDashNo && dashs[dashNo].length <= 0) {
                    dashNo++;
                    dashPos = 0;
                    if (dashNo >= nbDash) {
                        dashNo -= nbDash;
                    }
                }
                
                if (dashs[dashNo].length > 0) {
                    dashAbs += delta;
                    NR::Point pos = ((dTo->nDashAbs - dashAbs) * dTo->prevP+
                                     (dashAbs - oDashAbs)*dTo->curP) / (dTo->nDashAbs -	oDashAbs);
                    
                    double nw = (dTo->prevW * (dTo->nDashAbs - dashAbs) +
                                 dTo->curW * (dashAbs - oDashAbs)) / (dTo->nDashAbs - oDashAbs);
          
                    if (inGap && dashs[dashNo].gap == false) {
                        int nleftNo = dest->AddPoint(pos + nw * pnor);
                        int nrightNo = dest->AddPoint(pos - nw * pnor);
                        dest->AddEdge(nleftNo, nrightNo);
                        lastLeft = nleftNo;
                        lastRight = nrightNo;
            
                        inGap = false;
                        
                    } else if (inGap == false && dashs[dashNo].gap) {
                        
                        int nleftNo = dest->AddPoint(pos + nw * pnor);
                        int nrightNo = dest->AddPoint(pos - nw * pnor);
                        dest->AddEdge(nrightNo, nleftNo);
                        dest->AddEdge(lastRight, nrightNo);
                        dest->AddEdge(nleftNo, lastLeft);
                        lastLeft = -1;
                        lastRight = -1;
            
                        inGap = true;
                    }
                }
            } else {
                dashPos += dTo->nDashAbs - dashAbs;
                dashAbs = dTo->nDashAbs;
                break;
            }
        } while (dashNo != cDashNo);
    }
    
    inGap = dashs[dashNo].gap;
}


/*
  Local Variables:
  mode:c++
  c-file-style:"stroustrup"
  c-file-offsets:((innamespace . 0)(inline-open . 0))
  indent-tabs-mode:nil
  fill-column:99
  End:
*/
// vim: filetype=c++:expandtab:shiftwidth=4:tabstop=8:softtabstop=4 :
