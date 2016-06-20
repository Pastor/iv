package ru.iv.support.repository;

import org.springframework.data.repository.PagingAndSortingRepository;
import ru.iv.support.entity.QuestionSequence;

public interface QuestionSequenceRepository extends PagingAndSortingRepository<QuestionSequence, Long> {
}
