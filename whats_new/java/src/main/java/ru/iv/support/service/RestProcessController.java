package ru.iv.support.service;

import com.google.common.base.Stopwatch;
import com.google.common.collect.Sets;
import lombok.extern.slf4j.Slf4j;
import org.jetbrains.annotations.NotNull;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.domain.PageRequest;
import org.springframework.data.domain.Sort;
import org.springframework.http.MediaType;
import org.springframework.scheduling.annotation.Async;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.util.Assert;
import org.springframework.web.bind.annotation.*;
import ru.iv.support.WebNotifyController;
import ru.iv.support.entity.Answer;
import ru.iv.support.entity.Question;
import ru.iv.support.entity.QuestionResult;
import ru.iv.support.entity.QuestionSequence;
import ru.iv.support.repository.AnswerRepository;
import ru.iv.support.repository.QuestionRepository;
import ru.iv.support.repository.QuestionSequenceRepository;
import ru.iv.support.repository.ResultRepository;

import javax.annotation.PostConstruct;
import java.time.LocalDateTime;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import static java.text.MessageFormat.format;

@Slf4j
@SuppressWarnings("unused")
@RestController("restProcessController")
final class RestProcessController {

    private final Lock stopLock = new ReentrantLock();
    private final Condition stopSignal = stopLock.newCondition();
    private final AtomicBoolean isQuestionRunning = new AtomicBoolean(false);

    private final QuestionRepository questionRepository;
    private final ResultRepository resultRepository;
    private final AnswerRepository answerRepository;
    private final QuestionSequenceRepository sequenceRepository;

    @Autowired
    private WebNotifyController notifyController;

    @Autowired
    private RestProcessController(QuestionRepository questionRepository,
                                  ResultRepository resultRepository,
                                  AnswerRepository answerRepository,
                                  QuestionSequenceRepository sequenceRepository) {
        Assert.notNull(resultRepository);
        this.resultRepository = resultRepository;
        Assert.notNull(answerRepository);
        this.answerRepository = answerRepository;
        Assert.notNull(questionRepository);
        this.questionRepository = questionRepository;
        Assert.notNull(sequenceRepository);
        this.sequenceRepository = sequenceRepository;
    }

    @RequestMapping(path = "/sequences/create", consumes = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.POST)
    @ResponseBody
    @Transactional
    private void createSequence(@RequestBody final QuestionSequence sequence) {
        sequenceRepository.save(sequence);
    }

    @RequestMapping(path = "/sequences/list", produces = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.GET)
    @ResponseBody
    private Set<QuestionSequence> listSequences(@RequestParam(value = "limit", defaultValue = "100") int limit) {
        return Sets.newConcurrentHashSet(sequenceRepository.findAll(
                createPageable(limit, new Sort(Sort.Direction.ASC, "id"))));
    }

    @RequestMapping(path = "/questions/{idSequence}/list", produces = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.GET)
    @ResponseBody
    private Set<Question> listQuestions(@PathVariable("idSequence") long idSequence) {
        final QuestionSequence sequence = sequenceRepository.findOne(idSequence);
        if (sequence != null) {
            return Sets.newConcurrentHashSet(sequence.getQuestions());
        }
        return Sets.newConcurrentHashSet();
    }

    @RequestMapping(path = "/questions/{idSequence}/create", consumes = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.POST)
    @ResponseBody
    @Transactional
    private void createQuestion(@PathVariable("idSequence") long idSequence, @RequestBody final Question question) {
        final QuestionSequence sequence = sequenceRepository.findOne(idSequence);
        question.setSequence(sequence);
        questionRepository.save(question);
    }

    @RequestMapping(path = "/questions/{id}", produces = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.GET)
    @ResponseBody
    @Transactional
    private Question getQuestion(@PathVariable("id") long id) {
        return questionRepository.findOne(id);
    }


    @RequestMapping(path = "/results/list", produces = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.GET)
    @ResponseBody
    private Set<QuestionResult> listResults(@RequestParam(value = "limit", defaultValue = "100") int limit) {
        return Sets.newConcurrentHashSet(resultRepository.findAll(
                createPageable(limit, new Sort(Sort.Direction.ASC, "id"))));
    }

    @NotNull
    private static PageRequest createPageable(@RequestParam(value = "limit", defaultValue = "100") int limit, Sort id) {
        return new PageRequest(0, limit, id);
    }

    @RequestMapping(path = "/results/{id}", produces = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.GET)
    @ResponseBody
    private QuestionResult getResult(@PathVariable("id") long id) {
        return resultRepository.findOne(id);
    }

    @RequestMapping(path = "/results/current", produces = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.GET)
    @ResponseBody
    private QuestionResult currentSession() {
        final QuestionResult activate = resultRepository.findByActivate(true);
        if (activate == null)
            return resultRepository.findByName(QuestionResult.DEFAULT_NAME);
        return activate;
    }

    @RequestMapping(path = "/results/{id}/activate", consumes = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.PUT)
    @ResponseBody
    @Transactional
    private void activateResult(@PathVariable("id") long idSession) {
        Assert.isTrue(!isQuestionRunning.get(), "Уже запузен проуесс голосования");
        final QuestionResult questionResult = resultRepository.findOne(idSession);
        resultRepository.clearActivated();
        questionResult.setActivate(true);
        resultRepository.save(questionResult);
    }

    @RequestMapping(path = "/results/start/{idQuestion}", produces = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.PUT)
    @ResponseBody
    @Transactional
    private QuestionResult startResult(@PathVariable("idQuestion") long idQuestion) {
        resultRepository.clearActivated();
        return startResult(questionRepository.findOne(idQuestion));
    }

    @RequestMapping(path = "/results/stop", consumes = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.PUT)
    @ResponseBody
    private void stopResult() {
        stopSignal();
    }

    @RequestMapping(path = "/answer/{idResult}/list", produces = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.GET)
    @ResponseBody
    private Set<Answer> listAnswer(@PathVariable("idResult") long idResult, @RequestParam(value = "limit", defaultValue = "100") int limit) {
        final QuestionResult questionResult = resultRepository.findOne(idResult);
        if (questionResult != null) {
            return Sets.newConcurrentHashSet(questionResult.getAnswers());
        }
        return Sets.newHashSet();
    }

    @RequestMapping(path = "/answer/{idResult}/create", consumes = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.POST)
    @ResponseBody
    @Transactional
    private void createAnswer(@PathVariable("idResult") long idResult, @RequestBody final Answer answer) {
        final QuestionResult questionResult = resultRepository.findOne(idResult);
        answer.setQuestionResult(questionResult);
        answerRepository.save(answer);
    }

    private QuestionResult startResult(@javax.validation.constraints.NotNull Question question) {
        QuestionResult questionResult = new QuestionResult();
        questionResult.setQuestion(question);
        questionResult.setActivate(true);
        questionResult.setStartedAt(LocalDateTime.now());
        Long timeCount = question.getTimeCount();
        if (timeCount == null) {
            timeCount = Long.MAX_VALUE;
        }
        final QuestionResult save = resultRepository.save(questionResult);
        startAsyncResult(questionResult, timeCount);
        return save;
    }

    @Transactional
    @Async
    private void startAsyncResult(QuestionResult questionResult, long timeCount) {
        stopLock.lock();
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
        /**TODO: Notify */
        notifyController.broadcast("Stop");
    }

    private void tickUpdate(long elapsed) {
        /**TODO: Notify */
        notifyController.broadcast(format("Elapsed: {} ms", elapsed));
    }

    private boolean waitSignal() {
        try {
            return stopSignal.await(5000, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            return false;
        }
    }

    private void stopSignal() {
        stopLock.lock();
        try {
            isQuestionRunning.set(false);
            stopSignal.signalAll();
        } finally {
            stopLock.unlock();
        }
    }

    @PostConstruct
    @Transactional
    private void init() {
        final QuestionResult defaultQuestionResult = resultRepository.findByName(QuestionResult.DEFAULT_NAME);
        if (defaultQuestionResult == null) {
            final QuestionSequence sequence = new QuestionSequence();
            sequence.setName("Последовательность");
            sequenceRepository.save(sequence);
            final Question question = new Question();
            question.setSequence(sequence);
            question.setText("Результат по умолчанию");
            questionRepository.save(question);
            final QuestionResult questionResult = new QuestionResult();
            questionResult.setQuestion(question);
            questionResult.setName(QuestionResult.DEFAULT_NAME);
            resultRepository.save(questionResult);
        }
        resultRepository.clearActivated();
    }
}
