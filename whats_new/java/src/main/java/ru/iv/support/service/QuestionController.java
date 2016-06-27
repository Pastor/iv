package ru.iv.support.service;

import com.google.common.base.Stopwatch;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.scheduling.annotation.Async;
import org.springframework.scheduling.annotation.EnableAsync;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import ru.iv.support.DeviceController;
import ru.iv.support.entity.QuestionResult;
import ru.iv.support.entity.RequestGroup;
import ru.iv.support.notify.Notify;
import ru.iv.support.notify.WebNotifyController;
import ru.iv.support.repository.ResultRepository;

import java.time.LocalDateTime;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
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

    @Value(value = "#{'${controller.question.signal_timeout:1500}'}")
    private long waitSignalTimeout;

    @Autowired
    private DeviceController deviceController;

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
        final List<RequestGroup> groups = questionResult.getQuestion().getSequence().getRequestGroups();
        final Iterator<RequestGroup> iterator = groups != null ?
                groups.iterator() :
                new LinkedList<RequestGroup>().iterator();
        isQuestionRunning.set(true);
        notifyController.notify(Notify.of(Notify.Type.QUESTION_START));
        resultController.start(questionResult);
        try {
            final Stopwatch stopwatch = Stopwatch.createStarted();
            while (isQuestionRunning.get()) {
                final boolean hasNext = iterator.hasNext();
                if (hasNext) {
                    final RequestGroup next = iterator.next();
                    deviceController.send(next.toDevice(), next.toDeviceQuery());
                }
                if (waitSignal())
                    continue;
                final long elapsed = stopwatch.elapsed(TimeUnit.MILLISECONDS);
                if (elapsed >= timeCount && !hasNext) {
                    isQuestionRunning.set(false);
                } else {
                    tickUpdate(elapsed, timeCount);
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

    private void tickUpdate(long msComplete, long msFullTime) {
        notifyController.notify(Notify.of(msComplete, msFullTime));
    }

    private boolean waitSignal() {
        try {
            return stopSignal.await(waitSignalTimeout, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            return false;
        }
    }

    boolean isStarted() {
        return isQuestionRunning.get();
    }

    void stop() {
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
