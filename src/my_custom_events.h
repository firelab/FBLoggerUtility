#ifndef MY_CUSTOM_EVENTS_H
#define MY_CUSTOM_EVENTS_H

#include <QEvent>

// Ccustom event identifiers
const QEvent::Type MY_PROGRESS_UPDATE_EVENT = static_cast<QEvent::Type>(QEvent::User + 1);
const QEvent::Type MY_CANCEL_WORK_EVENT = static_cast<QEvent::Type>(QEvent::User + 2);

class ProgressUpdateEvent : public QEvent
{
    public:
        ProgressUpdateEvent(const int progressValue):
            QEvent(MY_PROGRESS_UPDATE_EVENT),
            progress(progressValue)
        {
        }

        int getProgress() const
        {
            return progress;
        }

    private:
        int progress;
};

class CancelWorkEvent : public QEvent
{
    public:
        CancelWorkEvent():
            QEvent(MY_CANCEL_WORK_EVENT)
        {

        }
};

#endif // MY_CUSTOM_EVENTS_H
