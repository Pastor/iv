package ru.iv.support.repository;

import org.springframework.data.jpa.repository.Modifying;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.PagingAndSortingRepository;
import org.springframework.transaction.annotation.Transactional;
import ru.iv.support.entity.Session;

public interface SessionRepository extends PagingAndSortingRepository<Session, Long> {
    @Transactional
    @Modifying(clearAutomatically = true)
    @Query("UPDATE Session s SET s.activate = false")
    void clearActivated();

    Session findByName(String name);

    Session findByActivate(boolean activate);
}
