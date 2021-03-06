TARGET = harbour-bibsearch

CONFIG += sailfishapp

SOURCES += src/main.cpp src/searchenginelist.cpp \
    src/bibliographymodel.cpp ../../src/data/value.cpp \
    ../../src/data/entry.cpp ../../src/data/macro.cpp \
    ../../src/data/comment.cpp ../../src/data/file.cpp \
    ../../src/data/preamble.cpp ../../src/data/element.cpp \
    ../../src/networking/internalnetworkaccessmanager.cpp \
    ../../src/networking/onlinesearch/onlinesearchabstract.cpp \
    ../../src/networking/onlinesearch/onlinesearchbibsonomy.cpp \
    ../../src/networking/onlinesearch/onlinesearchacmportal.cpp \
    ../../src/networking/onlinesearch/onlinesearchsciencedirect.cpp \
    ../../src/networking/onlinesearch/onlinesearchgooglescholar.cpp \
    ../../src/networking/onlinesearch/onlinesearchjstor.cpp \
    ../../src/networking/onlinesearch/onlinesearchspringerlink.cpp \
    ../../src/networking/onlinesearch/onlinesearchieeexplore.cpp \
    ../../src/networking/onlinesearch/onlinesearcharxiv.cpp \
    ../../src/networking/onlinesearch/onlinesearchingentaconnect.cpp \
    ../../src/networking/onlinesearch/onlinesearchpubmed.cpp \
    ../../src/global/kbibtex.cpp \
    ../../src/io/encoderxml.cpp ../../src/io/encoder.cpp \
    ../../src/io/encoderlatex.cpp \
    ../../src/io/fileimporter.cpp \
    ../../src/io/fileimporterbibtex.cpp \
    ../../src/io/xsltransform.cpp \
    ../../src/config/preferences.cpp \
    ../../src/config/bibtexfields.cpp \
    ../../src/config/bibtexentries.cpp \
    $${OUT_PWD}/src/config/logging_config.cpp \
    $${OUT_PWD}/src/networking/logging_networking.cpp \
    $${OUT_PWD}/src/data/logging_data.cpp $${OUT_PWD}/src/io/logging_io.cpp

HEADERS += src/bibliographymodel.h src/searchenginelist.h \
    src/kbibtexnamespace.h ../../src/data/entry.h \
    ../../src/data/macro.h ../../src/data/comment.h \
    ../../src/data/file.h ../../src/data/preamble.h \
    ../../src/data/value.h ../../src/data/element.h \
    ../../src/networking/internalnetworkaccessmanager.h \
    ../../src/networking/onlinesearch/onlinesearchabstract.h \
    ../../src/networking/onlinesearch/onlinesearchbibsonomy.h \
    ../../src/networking/onlinesearch/onlinesearchacmportal.h \
    ../../src/networking/onlinesearch/onlinesearchsciencedirect.h \
    ../../src/networking/onlinesearch/onlinesearchgooglescholar.h \
    ../../src/networking/onlinesearch/onlinesearchjstor.h \
    ../../src/networking/onlinesearch/onlinesearcharxiv.h \
    ../../src/networking/onlinesearch/onlinesearchingentaconnect.h \
    ../../src/networking/onlinesearch/onlinesearchspringerlink.h \
    ../../src/networking/onlinesearch/onlinesearchieeexplore.h \
    ../../src/networking/onlinesearch/onlinesearchpubmed.h \
    ../../src/global/kbibtex.h \
    ../../src/io/encoderxml.h ../../src/io/encoder.h \
    ../../src/io/encoderlatex.h ../../src/io/fileimporter.h \
    ../../src/io/fileimporterbibtex.h \
    ../../src/io/xsltransform.h \
    ../../src/config/preferences.h \
    ../../src/config/bibtexfields.h ../../src/config/bibtexentries.h

DISTFILES += qml/pages/SearchForm.qml qml/pages/EntryView.qml \
    qml/pages/AboutPage.qml qml/pages/BibliographyListView.qml \
    qml/cover/CoverPage.qml qml/BibSearch.qml \
    qml/pages/AboutPage.qml qml/pages/SearchEngineListView.qml \
    rpm/$${TARGET}.spec \
    rpm/$${TARGET}.yaml \
    translations/*.ts \
    $${TARGET}.desktop

RESOURCES += sailfishos_res.qrc

QT += xmlpatterns

DEFINES += KBIBTEXGLOBAL_EXPORT= KBIBTEXCONFIG_EXPORT= KBIBTEXDATA_EXPORT= KBIBTEXIO_EXPORT= KBIBTEXNETWORKING_EXPORT=

INCLUDEPATH +=  ../../src/global $${OUT_PWD}/src/global ../../src/config $${OUT_PWD}/src/config ../../src/data $${OUT_PWD}/src/data ../../src/io $${OUT_PWD}/src/io ../../src/networking ../../src/networking/onlinesearch $${OUT_PWD}/src/networking

# Do not forget to run  lupdate bibsearch.pro && lrelease bibsearch.pro  to update .qm files

TRANSLATIONS += \
    translations/$${TARGET}-de.ts \
    translations/$${TARGET}-en.ts
CODECFORTR = UTF-8


xslt.files = ../../xslt/pam2bibtex.xsl ../../xslt/ieeexploreapiv1-to-bibtex.xsl \
    ../../xslt/arxiv2bibtex.xsl ../../xslt/pubmed2bibtex.xsl
xslt.path = /usr/share/$${TARGET}
INSTALLS += xslt

icon86.files = icons/86/$${TARGET}.png
icon86.path = /usr/share/icons/hicolor/86x86/apps/
INSTALLS += icon86
icon108.files = icons/108/$${TARGET}.png
icon108.path = /usr/share/icons/hicolor/108x108/apps/
INSTALLS += icon108
icon128.files = icons/128/$${TARGET}.png
icon128.path = /usr/share/icons/hicolor/128x128/apps/
INSTALLS += icon128
icon172.files = icons/172/$${TARGET}.png
icon172.path = /usr/share/icons/hicolor/172x172/apps/
INSTALLS += icon172
icon256.files = icons/256/$${TARGET}.png
icon256.path = /usr/share/icons/hicolor/256x256/apps/
INSTALLS += icon256
