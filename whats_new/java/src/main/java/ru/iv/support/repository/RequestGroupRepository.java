package ru.iv.support.repository;

import org.springframework.data.repository.PagingAndSortingRepository;
import ru.iv.support.entity.RequestGroup;

public interface RequestGroupRepository extends PagingAndSortingRepository<RequestGroup, Long> {
}
