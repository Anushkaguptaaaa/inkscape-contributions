; #######################################
; german.nsh
; german language strings for inkscape installer
; windows code page: 1252
; Authors:
; Adib Taraben theAdib@yahoo.com
;
!insertmacro MUI_LANGUAGE "German"

; Product name
LangString lng_Caption   ${LANG_GERMAN}  "${PRODUCT_NAME} -- Open Source SVG-Vektorillustrator"

; Button text "Next >" for the license page
LangString lng_LICENSE_BUTTON   ${LANG_GERMAN} "Weiter >"

; Bottom text for the license page
LangString lng_LICENSE_BOTTOM_TEXT   ${LANG_GERMAN} "$(^Name) wird unter der GNU General Public License (GPL) ver�ffentlicht. Die Lizenz dient hier nur der Information. $_CLICK"

; Full install type
LangString lng_Full $(LANG_GERMAN) "Komplett"

; Optimal install type
LangString lng_Optimal $(LANG_GERMAN) "Optimal"

; Minimal install type
LangString lng_Minimal $(LANG_GERMAN) "Minimal"


; Core section
LangString lng_Core $(LANG_GERMAN) "${PRODUCT_NAME} Vektorillustrator (erforderlich)"

; Core section description
LangString lng_CoreDesc $(LANG_GERMAN) "${PRODUCT_NAME} Basis-Dateien und -DLLs"


; GTK+ section
LangString lng_GTKFiles $(LANG_GERMAN) "GTK+ Runtime Umgebung (erforderlich)"

; GTK+ section description
LangString lng_GTKFilesDesc $(LANG_GERMAN) "Ein Multi-Plattform GUI Toolkit, verwendet von ${PRODUCT_NAME}"


; shortcuts section
LangString lng_Shortcuts $(LANG_GERMAN) "Verkn�pfungen"

; shortcuts section description
LangString lng_ShortcutsDesc $(LANG_GERMAN) "Verkn�pfungen zum Start von ${PRODUCT_NAME}"

; multi user installation
LangString lng_Alluser  ${LANG_GERMAN}  "f�r Alle Benutzer"

; multi user installation description
LangString lng_AlluserDesc  ${LANG_GERMAN}  "Installiert diese Anwendung f�r alle Benutzer dieses Computers (all users)"

; Start Menu  section
LangString lng_Startmenu $(LANG_GERMAN) "Startmen�"

; Start Menu section description
LangString lng_StartmenuDesc $(LANG_GERMAN) "Erstellt einen Eintrag f�r ${PRODUCT_NAME} im Startmen�"

; Desktop section
LangString lng_Desktop $(LANG_GERMAN) "Desktop"

; Desktop section description
LangString lng_DesktopDesc $(LANG_GERMAN) "Erstellt eine Verkn�pfung zu ${PRODUCT_NAME} auf dem Desktop"

; Quick launch section
LangString lng_Quicklaunch $(LANG_GERMAN) "Schnellstartleiste"

; Quick launch section description
LangString lng_QuicklaunchDesc $(LANG_GERMAN) "Erstellt eine Verkn�pfung zu ${PRODUCT_NAME} auf der Schnellstartleiste"

; File type association for editing
LangString lng_SVGWriter    ${LANG_GERMAN}  "�ffne SVG Dateien mit ${PRODUCT_NAME}"

;LangString lng_UseAs ${LANG_ENGLISH} "Select ${PRODUCT_NAME} as default application for:"
LangString lng_SVGWriterDesc    ${LANG_GERMAN}  "W�hlen Sie ${PRODUCT_NAME} als Standardanwendung f�r SVG Dateien"

; Context Menu
LangString lng_ContextMenu ${LANG_GERMAN} "Kontext-Men�"

; Context Menu description
LangString lng_ContextMenuDesc ${LANG_GERMAN} "F�gt ${PRODUCT_NAME} in das Kontext-Men� f�r SVG Dateien ein"


; Additional Files section
LangString lng_Addfiles $(LANG_GERMAN) "weitere Dateien"

; additional files section dscription
LangString lng_AddfilesDesc $(LANG_GERMAN) "weitere Dateien"

; Examples section
LangString lng_Examples $(LANG_GERMAN) "Beispiele"

; Examples section dscription
LangString lng_ExamplesDesc $(LANG_GERMAN) "Beispiele mit ${PRODUCT_NAME}"

; Tutorials section
LangString lng_Tutorials $(LANG_GERMAN) "Tutorials"

; Tutorials section dscription
LangString lng_TutorialsDesc $(LANG_GERMAN) "Tutorials f�r die Benutzung mit ${PRODUCT_NAME}"


; Languages section
LangString lng_Languages $(LANG_GERMAN) "�bersetzungen"

; Languages section dscription
LangString lng_LanguagesDesc $(LANG_GERMAN) "Installiert verschiedene �bersetzungen f�r ${PRODUCT_NAME}"

LangString lng_am $(LANG_ENGLISH) "am  Amharisch"
LangString lng_az $(LANG_ENGLISH) "az  Aserbaidschanisch"
LangString lng_be $(LANG_ENGLISH) "be  Wei�russisch"
LangString lng_ca $(LANG_ENGLISH) "ca  Katalanisch"
LangString lng_cs $(LANG_ENGLISH) "cs  Tschechisch"
LangString lng_da $(LANG_ENGLISH) "da  D�nisch"
LangString lng_de $(LANG_ENGLISH) "de  Deutsch"
LangString lng_el $(LANG_ENGLISH) "el  Griechisch"
LangString lng_en $(LANG_ENGLISH) "en  Englisch"
LangString lng_es $(LANG_ENGLISH) "es  Spanisch"
LangString lng_es_MX $(LANG_ENGLISH) "es_MX  Spanisch-Mexio"
LangString lng_et $(LANG_ENGLISH) "es  Estonisch"
LangString lng_fr $(LANG_ENGLISH) "fr  Franz�sisch"
LangString lng_ga $(LANG_ENGLISH) "ga  Irisch"
LangString lng_gl $(LANG_ENGLISH) "gl  Galizisch"
LangString lng_hu $(LANG_ENGLISH) "hu  Ungarisch"
LangString lng_it $(LANG_ENGLISH) "it  Italienisch"
LangString lng_ja $(LANG_ENGLISH) "ja  Japanisch"
LangString lng_mk $(LANG_ENGLISH) "mk  Mazedonisch"
LangString lng_nb $(LANG_ENGLISH) "nb  Norwegisch-Bokmal"
LangString lng_nl $(LANG_ENGLISH) "nl  Holl�ndisch"
LangString lng_nn $(LANG_ENGLISH) "nn  Nynorsk-Norwegisch"
LangString lng_pa $(LANG_ENGLISH) "pa  Panjabi"
LangString lng_pl $(LANG_ENGLISH) "po  Polnisch"
LangString lng_pt $(LANG_ENGLISH) "pt  Portugiesisch"
LangString lng_pt_BR $(LANG_ENGLISH) "pt_BR  Portugiesisch Brazilien"
LangString lng_ru $(LANG_ENGLISH) "ru  Russisch"
LangString lng_sk $(LANG_ENGLISH) "sk  Slowakisch"
LangString lng_sl $(LANG_ENGLISH) "sl  Slowenisch"
LangString lng_sr $(LANG_ENGLISH) "sr  Serbisch"
LangString lng_sr@Latn $(LANG_ENGLISH) "sr@Latn Serbisch mit lateinischen Buchstaben"
LangString lng_sv $(LANG_ENGLISH) "sv  Schwedisch"
LangString lng_tr $(LANG_ENGLISH) "tr  T�rkisch"
LangString lng_uk $(LANG_ENGLISH) "uk  Ukrainisch"
LangString lng_zh_CN $(LANG_ENGLISH) "zh_CH  Chinesisch (vereinfacht)"


; uninstallation options
;LangString lng_UInstOpt   ${LANG_ENGLISH} "Uninstallation Options"
LangString lng_UInstOpt   ${LANG_GERMAN} "Deinstallations Optionen"

; uninstallation options subtitle
;LangString lng_UInstOpt1  ${LANG_ENGLISH} "Please make your choices for additional options"
LangString lng_UInstOpt1  ${LANG_GERMAN} "Bitte w�hlen Sie die optionalen Deinstalltionsparameter"

; Ask to purge the personal preferences
;LangString lng_PurgePrefs ${LANG_ENGLISH} "Keep Inkscape preferences"
LangString lng_PurgePrefs ${LANG_GERMAN}  "behalte pers�nliche Inkscape-Vorgaben"
