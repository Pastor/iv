package ru.iv.support.repository;

import org.springframework.data.domain.Pageable;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.PagingAndSortingRepository;
import org.springframework.data.repository.query.Param;
import ru.iv.support.entity.Answer;
import ru.iv.support.entity.QuestionResult;

import java.util.List;

public interface AnswerRepository extends PagingAndSortingRepository<Answer, Long> {

    @Query("SELECT e FROM Answer e WHERE e.questionResult = :result")
    List<Answer> listEventsBySession(@Param("result") QuestionResult questionResult, Pageable pageable);
}
