package ru.iv.support.repository;

import org.springframework.data.repository.PagingAndSortingRepository;
import ru.iv.support.entity.Profile;

public interface ProfileRepository extends PagingAndSortingRepository<Profile, Long> {
}
