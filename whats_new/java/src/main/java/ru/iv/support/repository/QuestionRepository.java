package ru.iv.support.repository;

import org.springframework.data.repository.PagingAndSortingRepository;
import ru.iv.support.entity.Question;

public interface QuestionRepository extends PagingAndSortingRepository<Question, Long> {
}
