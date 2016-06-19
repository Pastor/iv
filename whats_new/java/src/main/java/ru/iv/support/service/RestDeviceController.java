package ru.iv.support.service;

import com.google.common.io.CharStreams;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.core.task.TaskExecutor;
import org.springframework.http.MediaType;
import org.springframework.transaction.annotation.Transactional;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;
import org.springframework.web.bind.annotation.ResponseBody;
import org.springframework.web.bind.annotation.RestController;
import ru.iv.support.*;
import ru.iv.support.entity.Session;
import ru.iv.support.repository.EventRepository;
import ru.iv.support.repository.SessionRepository;

import javax.annotation.PostConstruct;
import javax.annotation.PreDestroy;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.Set;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

@SuppressWarnings("unused")
@RestController("deviceController")
final class RestDeviceController {
    private static final Gson GSON = new GsonBuilder().setPrettyPrinting().create();
    private final AtomicBoolean isRunnableEvents = new AtomicBoolean(true);


    @Autowired
    private SessionRepository sessionRepository;

    @Autowired
    private EventRepository eventRepository;

    @Autowired
    @Qualifier("defaultTaskExecutor")
    private TaskExecutor executor;

    @Autowired
    private DeviceController deviceController;

    @Autowired
    private WebDeviceController webController;

    @RequestMapping(path = "/version", produces = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.GET)
    @ResponseBody
    public String version() throws IOException, URISyntaxException {
        final URL resource = RestDeviceController.class.getResource("/version.json");
        return CharStreams.toString(new InputStreamReader(resource.openStream()));
    }

    @RequestMapping(path = "/devices", produces = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.GET)
    @ResponseBody
    public Set<DeviceInfo> devices() throws IOException, URISyntaxException {
        return deviceController.listDevices();
    }

    @RequestMapping(path = "/", method = RequestMethod.GET, produces = MediaType.TEXT_HTML_VALUE)
    public String index() throws IOException {
        final URL resource = RestDeviceController.class.getResource("/public/index.html");
        return CharStreams.toString(new InputStreamReader(resource.openStream()));
    }

    @PostConstruct
    private void construct() {
        executor.execute(() -> {
            while (isRunnableEvents.get()) {
                final Event event = next(deviceController.events());
                if (event != null) {
                    final String message = GSON.toJson(event, Event.class);
                    webController.broadcast(message);
                    System.out.println(event);
                    if (event.type == Event.Type.PACKETS) {
                        saveEvent(event);
                    }
                }
            }
        });
    }

    @Transactional
    private void saveEvent(Event event) {
        Session activate = sessionRepository.findByActivate(true);
        if (activate == null) {
            activate = sessionRepository.findByName(Session.DEFAULT_NAME);
        }
        for (Packet packet : event.packets) {
            final ru.iv.support.entity.Event ev =
                    ru.iv.support.entity.Event.of(activate, event.device, event.firmware, packet);
            eventRepository.save(ev);
        }
    }

    @PreDestroy
    private void destroy() {
        isRunnableEvents.set(false);
    }

    private static Event next(BlockingQueue<Event> events) {
        try {
            return events.poll(10000, TimeUnit.MILLISECONDS);
        } catch (Exception ex) {
            return null;
        }
    }
}
