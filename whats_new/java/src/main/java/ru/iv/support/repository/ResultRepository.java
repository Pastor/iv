package ru.iv.support.repository;

import org.springframework.data.jpa.repository.Modifying;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.PagingAndSortingRepository;
import org.springframework.transaction.annotation.Transactional;
import ru.iv.support.entity.QuestionResult;

public interface ResultRepository extends PagingAndSortingRepository<QuestionResult, Long> {
    @Transactional
    @Modifying(clearAutomatically = true)
    @Query("UPDATE QuestionResult s SET s.activate = false")
    void clearActivated();

    QuestionResult findByName(String name);

    QuestionResult findByActivate(boolean activate);
}
