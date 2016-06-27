package ru.iv.support.notify;

final class ProcessNotify extends Notify {
    private final long msComplete;
    private final long msFullTime;

    ProcessNotify(long msComplete, long msFullTime) {
        super(Type.QUESTION_PROCESS);
        this.msComplete = msComplete;
        this.msFullTime = msFullTime;
    }
}
