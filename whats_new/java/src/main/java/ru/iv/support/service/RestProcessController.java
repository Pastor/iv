package ru.iv.support.service;

import com.google.common.collect.Sets;
import lombok.extern.slf4j.Slf4j;
import org.jetbrains.annotations.NotNull;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.data.domain.PageRequest;
import org.springframework.data.domain.Sort;
import org.springframework.http.MediaType;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.util.Assert;
import org.springframework.web.bind.annotation.*;
import ru.iv.support.entity.Event;
import ru.iv.support.entity.Profile;
import ru.iv.support.entity.Session;
import ru.iv.support.repository.EventRepository;
import ru.iv.support.repository.ProfileRepository;
import ru.iv.support.repository.SessionRepository;

import javax.annotation.PostConstruct;
import java.util.Set;

@Slf4j
@SuppressWarnings("unused")
@RestController("restProcessController")
final class RestProcessController {
    private final ProfileRepository profileRepository;
    private final SessionRepository sessionRepository;
    private final EventRepository eventRepository;

    @Autowired
    private RestProcessController(ProfileRepository profileRepository, SessionRepository sessionRepository, EventRepository eventRepository) {
        Assert.notNull(sessionRepository);
        this.sessionRepository = sessionRepository;
        Assert.notNull(eventRepository);
        this.eventRepository = eventRepository;
        Assert.notNull(profileRepository);
        this.profileRepository = profileRepository;
    }

    @RequestMapping(path = "/profiles/list", produces = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.GET)
    @ResponseBody
    private Set<Profile> listProfiles(@RequestParam(value = "limit", defaultValue = "100") int limit) {
        return Sets.newConcurrentHashSet(profileRepository.findAll(createPageable(limit, new Sort(Sort.Direction.ASC, "name"))));
    }

    @RequestMapping(path = "/profiles/create", consumes = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.POST)
    @ResponseBody
    @Transactional
    private void createProfile(@RequestBody final Profile profile) {
        profileRepository.save(profile);
    }

    @RequestMapping(path = "/profiles/{id}", produces = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.GET)
    @ResponseBody
    @Transactional
    private Profile getProfile(@PathVariable("id") long id) {
        return profileRepository.findOne(id);
    }


    @RequestMapping(path = "/sessions/list", produces = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.GET)
    @ResponseBody
    private Set<Session> listSessions(@RequestParam(value = "limit", defaultValue = "100") int limit) {
        return Sets.newConcurrentHashSet(sessionRepository.findAll(
                createPageable(limit, new Sort(Sort.Direction.ASC, "id"))));
    }

    @NotNull
    private static PageRequest createPageable(@RequestParam(value = "limit", defaultValue = "100") int limit, Sort id) {
        return new PageRequest(0, limit, id);
    }

    @RequestMapping(path = "/sessions/{id}", produces = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.GET)
    @ResponseBody
    private Session listSessions(@PathVariable("id") long id) {
        return sessionRepository.findOne(id);
    }

    @RequestMapping(path = "/sessions/create", consumes = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.POST)
    @ResponseBody
    @Transactional
    private void createSession(@RequestBody final Session session) {
        sessionRepository.save(session);
    }

    @RequestMapping(path = "/sessions/{id}/activate", consumes = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.PUT)
    @ResponseBody
    @Transactional
    private void activateSession(@PathVariable("id") long idSession) {
        final Session session = sessionRepository.findOne(idSession);
        sessionRepository.clearActivated();
        session.setActivate(true);
        sessionRepository.save(session);
    }

    @RequestMapping(path = "/sessions/deactivate", consumes = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.PUT)
    @ResponseBody
    @Transactional
    private void deactivateSession() {
        sessionRepository.clearActivated();
    }

    @RequestMapping(path = "/events/{idSession}/list", produces = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.GET)
    @ResponseBody
    private Set<Event> listEvents(@PathVariable("idSession") long idSession, @RequestParam(value = "limit", defaultValue = "100") int limit) {
        final Session session = sessionRepository.findOne(idSession);
        if (session != null) {
            return Sets.newConcurrentHashSet(eventRepository.listEventsBySession(session, createPageable(limit, new Sort("id"))));
        }
        return Sets.newHashSet();
    }

    @RequestMapping(path = "/events/{idSession}/create", consumes = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.POST)
    @ResponseBody
    @Transactional
    private void createEvent(@PathVariable("idSession") long idSession, @RequestBody final Event event) {
        final Session session = sessionRepository.findOne(idSession);
        event.setSession(session);
        eventRepository.save(event);
    }

    @PostConstruct
    @Transactional
    private void init() {
        final Session defaultSession = sessionRepository.findByName(Session.DEFAULT_NAME);
        if (defaultSession == null) {
            final Session session = new Session();
            session.setName(Session.DEFAULT_NAME);
            sessionRepository.save(session);
        }
        sessionRepository.clearActivated();
    }
}
