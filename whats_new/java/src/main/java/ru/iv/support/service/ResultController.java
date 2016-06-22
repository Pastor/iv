package ru.iv.support.service;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;
import ru.iv.support.Device;
import ru.iv.support.Firmware;
import ru.iv.support.Packet;
import ru.iv.support.entity.Answer;
import ru.iv.support.entity.QuestionResult;
import ru.iv.support.repository.AnswerRepository;
import ru.iv.support.repository.ResultRepository;

import java.util.concurrent.atomic.AtomicReference;

@Service("resultController")
class ResultController {
    private final AtomicReference<QuestionResult> currentResult = new AtomicReference<>();

    private final ResultRepository resultRepository;
    private final AnswerRepository answerRepository;

    @Autowired
    public ResultController(ResultRepository resultRepository,
                            AnswerRepository answerRepository) {
        this.resultRepository = resultRepository;
        this.answerRepository = answerRepository;
    }

    public void start(QuestionResult result) {
        currentResult.set(result);
    }

    public void complete() {
        final QuestionResult questionResult = resultRepository.findByName(QuestionResult.DEFAULT_NAME);
        currentResult.set(questionResult);
    }

    @Transactional
    public void process(Device device, Firmware firmware, Packet[] packets) {
        for (Packet packet : packets) {
            final Answer ev =
                    Answer.of(currentResult.get(), device, firmware, packet);
            answerRepository.save(ev);
        }
    }
}
