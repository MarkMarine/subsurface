#include <string>

#include "templatelayout.h"
#include "helpers.h"
#include "display.h"

int getTotalWork(print_options *printOptions)
{
	if (printOptions->print_selected) {
		// return the correct number depending on all/selected dives
		// but don't return 0 as we might divide by this number
		return amount_selected ? amount_selected : 1;
	}
	int dives = 0, i;
	struct dive *dive;
	for_each_dive (i, dive) {
		dives++;
	}
	return dives;
}

TemplateLayout::TemplateLayout(print_options *PrintOptions, template_options *templateOptions) :
	m_engine(NULL)
{
	this->PrintOptions = PrintOptions;
	this->templateOptions = templateOptions;
}

TemplateLayout::~TemplateLayout()
{
	delete m_engine;
}

QString TemplateLayout::generate()
{
	int progress = 0;
	int totalWork = getTotalWork(PrintOptions);
	QString templateName;

	QString htmlContent;
	m_engine = new Grantlee::Engine(this);

	QSharedPointer<Grantlee::FileSystemTemplateLoader> m_templateLoader =
		QSharedPointer<Grantlee::FileSystemTemplateLoader>(new Grantlee::FileSystemTemplateLoader());
	m_templateLoader->setTemplateDirs(QStringList() << getSubsurfaceDataPath("printing_templates"));
	m_engine->addTemplateLoader(m_templateLoader);

	Grantlee::registerMetaType<Dive>();
	Grantlee::registerMetaType<template_options>();

	QVariantHash mapping;
	QVariantList diveList;

	struct dive *dive;
	int i;
	for_each_dive (i, dive) {
		//TODO check for exporting selected dives only
		if (!dive->selected && PrintOptions->print_selected)
			continue;
		Dive d(dive);
		diveList.append(QVariant::fromValue(d));
		progress++;
		emit progressUpdated(progress * 100.0 / totalWork);
	}
	mapping.insert("dives", diveList);
	mapping.insert("template_options", QVariant::fromValue(*templateOptions));

	Grantlee::Context c(mapping);

	if (PrintOptions->p_template == print_options::ONE_DIVE) {
		templateName = "one_dive.html";
	} else if (PrintOptions->p_template == print_options::TWO_DIVE) {
		templateName = "two_dives.html";
	} else if (PrintOptions->p_template == print_options::CUSTOM) {
		templateName = "custom.html";
	}
	Grantlee::Template t = m_engine->loadByName(templateName);
	if (!t || t->error()) {
		qDebug() << "Can't load template";
		return htmlContent;
	}

	htmlContent = t->render(&c);

	if (t->error()) {
		qDebug() << "Can't render template";
		return htmlContent;
	}
	return htmlContent;
}

QString TemplateLayout::readTemplate(QString template_name)
{
	QFile qfile(getSubsurfaceDataPath("printing_templates") + QDir::separator() + template_name);
	if (qfile.open(QFile::ReadOnly | QFile::Text)) {
		QTextStream in(&qfile);
		return in.readAll();
	}
	return "";
}

void TemplateLayout::writeTemplate(QString template_name, QString grantlee_template)
{
	QFile qfile(getSubsurfaceDataPath("printing_templates") + QDir::separator() + template_name);
	if (qfile.open(QFile::ReadWrite | QFile::Text)) {
		qfile.write(grantlee_template.toUtf8().data());
		qfile.close();
	}
}

Dive::Dive() :
	m_number(-1),
	dive(NULL)
{
}

Dive::~Dive()
{
}

int Dive::number() const
{
	return m_number;
}

QString Dive::date() const
{
	return m_date;
}

QString Dive::time() const
{
	return m_time;
}

QString Dive::location() const
{
	return m_location;
}

QString Dive::duration() const
{
	return m_duration;
}

QString Dive::depth() const
{
	return m_depth;
}

QString Dive::divemaster() const
{
	return m_divemaster;
}

QString Dive::buddy() const
{
	return m_buddy;
}

QString Dive::airTemp() const
{
	return m_airTemp;
}

QString Dive::waterTemp() const
{
	return m_waterTemp;
}

QString Dive::notes() const
{
	return m_notes;
}

void Dive::put_divemaster()
{
	if (!dive->divemaster)
		m_divemaster = "";
	else
		m_divemaster = dive->divemaster;
}

void Dive::put_date_time()
{
	QDateTime localTime = QDateTime::fromTime_t(dive->when - gettimezoneoffset(displayed_dive.when));
	localTime.setTimeSpec(Qt::UTC);
	m_date = localTime.date().toString(QString::fromUtf8("MMM dd, yyyy"));
	m_time = localTime.time().toString(QString::fromUtf8("hh:mm a"));
}

void Dive::put_location()
{
	m_location = QString::fromUtf8(get_dive_location(dive));
}

void Dive::put_depth()
{
	m_depth = get_depth_string(dive->dc.maxdepth.mm, true, true);
}

void Dive::put_duration()
{
	m_duration = QString::number(((dive->duration.seconds) / 60)) + QString::fromUtf8(" min");
}

void Dive::put_buddy()
{
	if (!dive->buddy)
		m_buddy = "";
	else
		m_buddy = dive->buddy;
}

void Dive::put_temp()
{
	m_airTemp = get_temperature_string(dive->airtemp, true);
	m_waterTemp = get_temperature_string(dive->watertemp, true);
}

void Dive::put_notes()
{
	m_notes = QString::fromUtf8(dive->notes);
}
