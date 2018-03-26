#include "anime_panel.h"
#include "ui_anime_panel.h"

#include <QSpinBox>

#include "../src/models/user.h"
#include "../src/utilities/file_downloader.h"

AnimePanel::AnimePanel(Media *media, QWidget *parent)
    : QDialog(parent), ui(new Ui::AnimePanel), media(media) {
  ui->setupUi(this);
  ui->tabWidget->setCurrentIndex(0);

  QStringList combinedGenreTags = media->genres();

  for (auto &&tag : media->tags()) {
    combinedGenreTags.append(tag);
  }

  qSort(combinedGenreTags);

  ui->title->setText(media->title());
  ui->description->setText(media->description());
  ui->synonyms->setText(media->synonyms().join("\n"));
  ui->episodes->setText(QString::number(media->episodes()));
  ui->status->setText(tr(qPrintable(media->airingStatus())));
  ui->genres->setText(combinedGenreTags.join(", "));

  FileDownloader *f = new FileDownloader(media->coverImage());

  connect(f, &FileDownloader::downloaded, [this, f]() {
    auto width = ui->coverImage->width();
    QPixmap image;
    image.loadFromData(f->downloadedData());
    image = image.scaledToWidth(width, Qt::SmoothTransformation);
    ui->coverImage->setPixmap(image);
    f->deleteLater();
  });

  ui->listStatus->setCurrentText(media->listStatus());
  ui->progress->setValue(media->progress());

  if (media->episodes() > 0) {
    ui->progress->setMaximum(media->episodes());
  }

  ui->rewatched->setValue(media->repeat());
  ui->priority->setValue(media->priority());
  ui->isPrivate->setChecked(media->isPrivate());

  connect(ui->listStatus, &QComboBox::currentTextChanged, [this](QString v) {
    if (v == tr("CURRENT")) {
      v = "CURRENT";
    } else if (v == tr("COMPLETED")) {
      v = "COMPLETED";
    } else if (v == tr("DROPPED")) {
      v = "DROPPED";
    } else if (v == tr("PAUSED")) {
      v = "PAUSED";
    } else if (v == tr("REPEATING")) {
      v = "REPEATING";
    }

    changes["status"] = v;
  });

  auto overload = QOverload<int>::of(&QSpinBox::valueChanged);

  connect(ui->progress, overload, [this](int v) { changes["progress"] = v; });

  connect(ui->rewatched, overload, [this](int v) { changes["repeat"] = v; });

  connect(ui->priority, overload, [this](int v) { changes["priority"] = v; });

  connect(ui->isPrivate, &QCheckBox::clicked,
          [this](bool v) { changes["private"] = v; });

  connect(ui->notes, &QTextEdit::textChanged,
          [this]() { changes["notes"] = ui->notes->toPlainText(); });

  createScoreEditor();
}

AnimePanel::~AnimePanel() { delete ui; }

void AnimePanel::createScoreEditor() {
  auto &user = User::instance();
  auto scoreFormat = user.scoreFormat();
  auto overload = QOverload<int>::of(&QSpinBox::valueChanged);
  auto value = media->score();

  auto layout = new QHBoxLayout(ui->score);
  layout->setMargin(0);
  layout->setSpacing(0);

  if (scoreFormat == "POINT_10" || scoreFormat == "POINT_100") {
    QSpinBox *score_container = new QSpinBox(this);

    score_container->setMaximum(scoreFormat == "POINT_10" ? 10 : 100);
    score_container->setMinimum(0);
    score_container->setMinimumWidth(75);
    score_container->setValue((int)value);

    layout->addWidget(score_container);

    connect(score_container, overload,
            [this, scoreFormat](int v) { changes["score"] = v; });
  } else if (scoreFormat == "POINT_5") {
    QComboBox *score_container = new QComboBox(this);

    score_container->addItem("0 ★");
    score_container->addItem("1 ★");
    score_container->addItem("2 ★");
    score_container->addItem("3 ★");
    score_container->addItem("4 ★");
    score_container->addItem("5 ★");
    score_container->setMinimumWidth(75);

    score_container->setCurrentText(QString::number((int)value) + " ★");

    layout->addWidget(score_container);

    connect(score_container, &QComboBox::currentTextChanged, [this](QString v) {
      changes["score"] = v.left(v.length() - 2).toInt();
    });

  } else if (scoreFormat == "POINT_3") {
    QComboBox *score_container = new QComboBox(this);

    score_container->addItem("");
    score_container->addItem(":(");
    score_container->addItem(":|");
    score_container->addItem(":)");
    score_container->setMinimumWidth(75);

    switch ((int)value) {
      case 1:
        score_container->setCurrentText(":(");
        break;
      case 2:
        score_container->setCurrentText(":|");
        break;
      case 3:
        score_container->setCurrentText(":)");
        break;
      default:
        break;
    }

    layout->addWidget(score_container);

    connect(score_container, &QComboBox::currentTextChanged, [this](QString v) {
      changes["score"] = v == ":)" ? 3 : v == ":|" ? 2 : v == ":(" ? 1 : 0;
    });
  } else if (scoreFormat == "POINT_10_DECIMAL") {
    QDoubleSpinBox *score_container = new QDoubleSpinBox(this);

    score_container->setMaximum(10.0);
    score_container->setMinimum(0);
    score_container->setDecimals(1);
    score_container->setSingleStep(0.1);
    score_container->setValue(value);
    score_container->setMinimumWidth(75);

    layout->addWidget(score_container);

    auto doverload = QOverload<double>::of(&QDoubleSpinBox::valueChanged);

    connect(score_container, doverload,
            [this](double v) { changes["score"] = v; });
  }

  ui->score->setLayout(layout);
}
