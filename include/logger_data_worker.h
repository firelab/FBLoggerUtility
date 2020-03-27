#ifndef LOGGER_DATA_WORKER_H
#define LOGGER_DATA_WORKER_H

#ifdef _OPENMP
#include <omp.h>
#endif

#include <QObject>
#include <QVector>

#include "mainwindow.h"
#include "logger_data_reader.h"
#include "my_custom_events.h"

class LoggerDataWorker : public QObject
{
    Q_OBJECT

public:
    LoggerDataWorker(QWidget* myParent);
    ~LoggerDataWorker();

protected:
    void customEvent(QEvent *event); // This overrides QObject::customEvent()

private:
    void handleCancelWorkEvent(const CancelWorkEvent* event);
   
public slots:
    void doWork(SharedData* sharedData);
    void cancelWork();

signals:
    void progress(int currentProgress);
    void resultReady();

private:
    QWidget* parent;

    volatile bool isWorkCancelled;
};

#endif // LOGGER_DATA_WORKER_H
