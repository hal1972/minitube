#include "SearchView.h"
#include "Constants.h"

static const QString recentKeywordsKey = "recentKeywords";
static const int PADDING = 40;

SearchView::SearchView(QWidget *parent) : QWidget(parent) {

    QFont biggerFont;
    biggerFont.setPointSize(biggerFont.pointSize()*1.5);

    QFont smallerFont;
    smallerFont.setPointSize(smallerFont.pointSize()*.85);
    smallerFont.setBold(true);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setMargin(0);

    // hidden message widget
    message = new QLabel(this);
    message->hide();
    mainLayout->addWidget(message);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setMargin(PADDING);
    mainLayout->addLayout(layout);

    QLabel *welcomeLabel =
            new QLabel("<h1>" +
                       tr("Welcome to <a href='%1'>%2</a>,").arg(Constants::WEBSITE, Constants::APP_NAME)
                       + "</h1>", this);
    welcomeLabel->setOpenExternalLinks(true);
    layout->addWidget(welcomeLabel);

    layout->addSpacing(PADDING);

    QLabel *tipLabel = new QLabel(tr("Enter a keyword to start watching videos."), this);
    tipLabel->setFont(biggerFont);
    layout->addWidget(tipLabel);

    QHBoxLayout *searchLayout = new QHBoxLayout();

    queryEdit = new SearchLineEdit(this);
    queryEdit->setFont(biggerFont);
    queryEdit->setMinimumWidth(300);
    queryEdit->sizeHint();
    queryEdit->setFocus(Qt::OtherFocusReason);
    // connect(queryEdit, SIGNAL(returnPressed()), this, SLOT(watch()));
    connect(queryEdit, SIGNAL(search(const QString&)), this, SLOT(watch(const QString&)));
    connect(queryEdit, SIGNAL(textChanged(const QString &)), this, SLOT(textChanged(const QString &)));
    searchLayout->addWidget(queryEdit);

    searchLayout->addSpacing(10);

    watchButton = new QPushButton(tr("Watch"), this);
    watchButton->setDefault(true);
    watchButton->setEnabled(false);
    connect(watchButton, SIGNAL(clicked()), this, SLOT(watch()));
    searchLayout->addWidget(watchButton);

    searchLayout->addStretch();

    layout->addItem(searchLayout);

    layout->addSpacing(PADDING);

    QHBoxLayout *otherLayout = new QHBoxLayout();

    recentKeywordsLayout = new QVBoxLayout();
    recentKeywordsLayout->setAlignment(Qt::AlignTop);
    recentKeywordsLabel = new QLabel(tr("Recent keywords").toUpper(), this);
    recentKeywordsLabel->hide();
    recentKeywordsLabel->setForegroundRole(QPalette::Dark);
    recentKeywordsLabel->setFont(smallerFont);
    recentKeywordsLayout->addWidget(recentKeywordsLabel);

    otherLayout->addLayout(recentKeywordsLayout);

    layout->addLayout(otherLayout);

    layout->addStretch();

    setLayout(mainLayout);

    // watchButton->setMinimumHeight(queryEdit->height());

    updateChecker = 0;
    checkForUpdate();
}

void SearchView::paintEvent(QPaintEvent * /*event*/) {

    QPainter painter(this);

#ifdef Q_WS_MAC
    QLinearGradient linearGrad(0, 0, 0, height());
    QPalette palette;
    linearGrad.setColorAt(0, palette.color(QPalette::Light));
    linearGrad.setColorAt(1, palette.color(QPalette::Midlight));
    painter.fillRect(0, 0, width(), height(), QBrush(linearGrad));
#endif

    QPixmap watermark = QPixmap(":/images/app.png");
    painter.setOpacity(.25);
    painter.drawPixmap(width() - watermark.width() - PADDING,
                       height() - watermark.height() - PADDING,
                       watermark.width(),
                       watermark.height(),
                       watermark);
}

void SearchView::updateRecentKeywords() {

    // cleanup
    QLayoutItem *item;
    while ((item = recentKeywordsLayout->takeAt(1)) != 0) {
        item->widget()->close();
        delete item;
    }

    // load
    QSettings settings;
    QStringList keywords = settings.value(recentKeywordsKey).toStringList();
    recentKeywordsLabel->setVisible(!keywords.isEmpty());
    foreach (QString keyword, keywords) {
        QLabel *itemLabel = new QLabel("<a href=\"" + keyword
                                       + "\" style=\"color:palette(text); text-decoration:none\">"
                                       + keyword + "</a>", this);
        itemLabel->setTextInteractionFlags(Qt::LinksAccessibleByKeyboard);
        connect(itemLabel, SIGNAL(linkActivated(QString)), this, SLOT(watch(QString)));
        recentKeywordsLayout->addWidget(itemLabel);
    }

}

void SearchView::watch() {
    QString query = queryEdit->text().trimmed();
    watch(query);
}

void SearchView::textChanged(const QString &text) {
    watchButton->setEnabled(!text.trimmed().isEmpty());
}

void SearchView::watch(QString query) {

    // check for empty query
    if (query.length() == 0) {
        queryEdit->setFocus(Qt::OtherFocusReason);
        return;
    }

    // save keyword
    QSettings settings;
    QStringList keywords = settings.value(recentKeywordsKey).toStringList();
    keywords.removeAll(query);
    keywords.prepend(query);
    while (keywords.size() > 10)
        keywords.removeLast();
    settings.setValue(recentKeywordsKey, keywords);

    // go!
    emit search(query);
}

void SearchView::checkForUpdate() {
    static const QString updateCheckKey = "updateCheck";

    // check every 24h
    QSettings settings;
    uint unixTime = QDateTime::currentDateTime().toTime_t();
    int lastCheck = settings.value(updateCheckKey).toInt();
    int secondsSinceLastCheck = unixTime - lastCheck;
    // qDebug() << "secondsSinceLastCheck" << unixTime << lastCheck << secondsSinceLastCheck;
    if (secondsSinceLastCheck < 86400) return;

    // check it out
    if (updateChecker) delete updateChecker;
    updateChecker = new UpdateChecker();
    connect(updateChecker, SIGNAL(newVersion(QString)),
            this, SLOT(gotNewVersion(QString)));
    updateChecker->checkForUpdate();
    settings.setValue(updateCheckKey, unixTime);

}

void SearchView::gotNewVersion(QString version) {
    message->setText(
            tr("A new version of %1 is available. Please <a href='%2'>update to version %3</a>")
            .arg(
                    Constants::APP_NAME,
                    QString(Constants::WEBSITE).append("#download"),
                    version)
            );
    message->setOpenExternalLinks(true);
    message->setMargin(10);
    message->setBackgroundRole(QPalette::ToolTipBase);
    message->setForegroundRole(QPalette::ToolTipText);
    message->setAutoFillBackground(true);
    message->show();
    if (updateChecker) delete updateChecker;
}
