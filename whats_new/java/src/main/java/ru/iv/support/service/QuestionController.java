package ru.iv.support.service;

import com.google.common.base.Stopwatch;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.scheduling.annotation.Async;
import org.springframework.scheduling.annotation.EnableAsync;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import ru.iv.support.entity.QuestionResult;
import ru.iv.support.notify.Notify;
import ru.iv.support.notify.WebNotifyController;
import ru.iv.support.repository.ResultRepository;

import java.time.LocalDateTime;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

@EnableAsync
@Service("questionController")
class QuestionController {
    private final Lock stopLock = new ReentrantLock();
    private final Condition stopSignal = stopLock.newCondition();
    private final AtomicBoolean isQuestionRunning = new AtomicBoolean(false);

    @Autowired
    private WebNotifyController notifyController;

    @Autowired
    private ResultRepository resultRepository;

    @Autowired
    private ResultController resultController;

    @Transactional
    @Async("questionTaskExecutor")
    public void start(QuestionResult questionResult, long timeCount) {
        stopLock.lock();
        isQuestionRunning.set(true);
        notifyController.notify(Notify.of(Notify.Type.QUESTION_START));
        resultController.start(questionResult);
        try {
            final Stopwatch stopwatch = Stopwatch.createStarted();
            while (isQuestionRunning.get()) {
                if (waitSignal())
                    continue;
                final long elapsed = stopwatch.elapsed(TimeUnit.MILLISECONDS);
                if (elapsed >= timeCount) {
                    isQuestionRunning.set(false);
                } else {
                    tickUpdate(elapsed);
                }
            }
        } finally {
            stopLock.unlock();
        }
        questionResult.setStoppedAt(LocalDateTime.now());
        questionResult.setActivate(false);
        resultRepository.save(questionResult);
        resultRepository.clearActivated();
        notifyController.notify(Notify.of(Notify.Type.QUESTION_COMPLETE));
        resultController.complete();
    }

    private void tickUpdate(long elapsed) {
        notifyController.notify(Notify.of(Notify.Type.QUESTION_PROCESS));
    }

    private boolean waitSignal() {
        try {
            return stopSignal.await(1000, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            return false;
        }
    }

    public boolean isStarted() {
        return isQuestionRunning.get();
    }

    public void stop() {
        stopLock.lock();
        try {
            notifyController.notify(Notify.of(Notify.Type.QUESTION_CANCEL));
            isQuestionRunning.set(false);
            stopSignal.signalAll();
        } finally {
            stopLock.unlock();
        }
    }
}
