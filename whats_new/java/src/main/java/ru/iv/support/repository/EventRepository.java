package ru.iv.support.repository;

import org.springframework.data.domain.Pageable;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.PagingAndSortingRepository;
import org.springframework.data.repository.query.Param;
import ru.iv.support.entity.Event;
import ru.iv.support.entity.Session;

import java.util.List;

public interface EventRepository extends PagingAndSortingRepository<Event, Long> {

    @Query("SELECT e FROM Event e WHERE e.session = :session")
    List<Event> listEventsBySession(@Param("session") Session session, Pageable pageable);
}
