package ru.iv.support.service;

import com.google.common.collect.Sets;
import lombok.extern.slf4j.Slf4j;
import org.jetbrains.annotations.NotNull;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.domain.PageRequest;
import org.springframework.data.domain.Sort;
import org.springframework.http.MediaType;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.util.Assert;
import org.springframework.web.bind.annotation.*;
import ru.iv.support.entity.*;
import ru.iv.support.notify.WebNotifyController;
import ru.iv.support.repository.*;

import javax.annotation.PostConstruct;
import java.time.LocalDateTime;
import java.util.Set;

@Slf4j
@SuppressWarnings("unused")
@RestController("restProcessController")
final class RestProcessController {

    private final QuestionRepository questionRepository;
    private final ResultRepository resultRepository;
    private final AnswerRepository answerRepository;
    private final QuestionSequenceRepository sequenceRepository;
    private final QuestionChoiceRepository choiceRepository;
    private final RequestGroupRepository requestGroupRepository;

    @Autowired
    private WebNotifyController notifyController;

    @Autowired
    private QuestionController questionController;

    @Autowired
    private ResultController resultController;

    @Autowired
    private RestProcessController(QuestionRepository questionRepository,
                                  ResultRepository resultRepository,
                                  AnswerRepository answerRepository,
                                  QuestionSequenceRepository sequenceRepository,
                                  QuestionChoiceRepository choiceRepository,
                                  RequestGroupRepository requestGroupRepository) {
        this.choiceRepository = choiceRepository;
        this.requestGroupRepository = requestGroupRepository;
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

    @RequestMapping(path = "/groups/{idSequence}/create", consumes = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.POST)
    @ResponseBody
    @Transactional
    private void createRequestGroup(@PathVariable("idSequence") long idSequence, @RequestBody final RequestGroup group) {
        final QuestionSequence sequence = sequenceRepository.findOne(idSequence);
        group.setSequence(sequence);
        requestGroupRepository.save(group);
    }

    @RequestMapping(path = "/groups/{idGroup}", method = RequestMethod.DELETE)
    private void deleteRequestGroup(@PathVariable("idGroup") long idGroup) {
        requestGroupRepository.delete(idGroup);
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
        Assert.isTrue(!questionController.isStarted(), "Уже запузен проуесс голосования");
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
        questionController.stop();
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
        questionController.start(questionResult, timeCount);
        return save;
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
            createTests();
        }
        resultRepository.clearActivated();
        resultController.complete();
    }

    private void createTests() {
        final QuestionSequence sequence = new QuestionSequence();
        sequence.setName("Ткстовая последовательность");
        sequenceRepository.save(sequence);
        final Question question = new Question();
        question.setSequence(sequence);
        question.setText("Главный вопрос");
        question.setTimeCount((long) 30000);
        questionRepository.save(question);
        final QuestionChoice choice1 = new QuestionChoice();
        choice1.setQuestion(question);
        choice1.setText("Вопрос №1");
        choice1.setOrder(1);
        choice1.setShowEnter("1");
        choice1.setDeviceEnter("1");
        choiceRepository.save(choice1);
        final QuestionChoice choice2 = new QuestionChoice();
        choice2.setQuestion(question);
        choice2.setText("Вопрос №2");
        choice2.setOrder(2);
        choice2.setShowEnter("2");
        choice2.setDeviceEnter("2");
        choiceRepository.save(choice2);
        final QuestionChoice choice3 = new QuestionChoice();
        choice3.setQuestion(question);
        choice3.setText("Вопрос №3");
        choice3.setOrder(3);
        choice3.setShowEnter("3");
        choice3.setDeviceEnter("3");
        choiceRepository.save(choice3);
    }
}
