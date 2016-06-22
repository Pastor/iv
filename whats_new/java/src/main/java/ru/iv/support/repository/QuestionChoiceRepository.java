package ru.iv.support.repository;

import org.springframework.data.repository.PagingAndSortingRepository;
import ru.iv.support.entity.QuestionChoice;

public interface QuestionChoiceRepository extends PagingAndSortingRepository<QuestionChoice, Long> {

}
