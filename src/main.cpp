#include <QApplication>
#include <QCoreApplication>
#include <QFile>
#include <QTimer>

#include "./ui/main_window.h"

#include "./clients/anilist.h"
#include "./detection/window_enumerator.h"
#include "./models/media_list.h"
#include "./models/user.h"
#include "./settings.h"
#include "./utilities/crash_handler.h"

int main(int argc, char *argv[]) {
#ifdef Q_OS_WIN
  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
#endif

  QCoreApplication::setOrganizationName("Kazakuri");
  QCoreApplication::setOrganizationDomain("shinjiru.me");
  QCoreApplication::setApplicationName("Shinjiru");

  QFile styleSheet(":/res/style.qss");

  if (!styleSheet.open(QFile::ReadOnly)) {
    // TODO
    return EXIT_FAILURE;
  }

  QString style = styleSheet.readAll();

  QApplication a(argc, argv);
  a.setStyleSheet(style);

#ifndef QT_DEBUG
  Breakpad::CrashHandler::instance()->Init(qApp->applicationDirPath());
#endif

  {
    QPixmap window_icon;
    QFile iconFile(":/res/icon.svg");
    iconFile.open(QFile::ReadOnly);
    QByteArray icon_data = iconFile.readAll();
    window_icon.loadFromData(icon_data);
    qApp->setWindowIcon(QIcon(window_icon));
  }

  Settings s;

  // TODO: language
  QTranslator qtTranslator;
  qtTranslator.load("qt_en", QLibraryInfo::location(QLibraryInfo::TranslationsPath));
  a.installTranslator(&qtTranslator);

  QTranslator shinjiruTranslator;
  shinjiruTranslator.load(":/lang/shinjiru_en");
  a.installTranslator(&shinjiruTranslator);

  MainWindow w;

  if (s.get(Setting::StartMinimized).toBool()) {
    if (s.get(Setting::MinimizeToTray).toBool()) {
      w.hide();
    } else {
      w.showMinimized();
    }
  } else {
    w.show();
  }

  AniList *anilist = &AniList::instance();

  WindowEnumerator::instance();

  QCoreApplication::connect(anilist, &AniList::authenticated, []() {
    User::instance().load();
    MediaList::instance().load();
  });

  QCoreApplication::connect(anilist, &AniList::reload, []() {
    User::instance().load();
    MediaList::instance().load();
  });

  anilist->grant();

  return a.exec();
}
