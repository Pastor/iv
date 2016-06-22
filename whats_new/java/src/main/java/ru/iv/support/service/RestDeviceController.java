package ru.iv.support.service;

import com.google.common.io.CharStreams;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.core.task.TaskExecutor;
import org.springframework.http.MediaType;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;
import org.springframework.web.bind.annotation.ResponseBody;
import org.springframework.web.bind.annotation.RestController;
import ru.iv.support.DeviceController;
import ru.iv.support.DeviceInfo;
import ru.iv.support.Event;
import ru.iv.support.notify.Notify;
import ru.iv.support.notify.WebNotifyController;
import ru.iv.support.repository.AnswerRepository;
import ru.iv.support.repository.ResultRepository;

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
    private ResultRepository resultRepository;

    @Autowired
    private AnswerRepository answerRepository;

    @Autowired
    private ResultController resultController;

    @Autowired
    @Qualifier("deviceTaskExecutor")
    private TaskExecutor executor;

    @Autowired
    private DeviceController deviceController;

    @Autowired
    private WebNotifyController webController;

    @RequestMapping(path = "/version", produces = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.GET)
    @ResponseBody
    public String version() throws IOException, URISyntaxException {
        final URL resource = RestDeviceController.class.getResource("/version.json");
        return CharStreams.toString(new InputStreamReader(resource.openStream()));
    }

    @RequestMapping(path = "/devices/list", produces = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.GET)
    @ResponseBody
    public String devices() throws IOException, URISyntaxException {
        final Set<DeviceInfo> devices = deviceController.listDevices();
        return GSON.toJson(devices.toArray(new DeviceInfo[devices.size()]), DeviceInfo[].class);
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
                    if (event.type == Event.Type.PACKETS) {
                        resultController.process(event.device, event.firmware, event.packets);
                    }
                    webController.notify(Notify.of(event));
                }
            }
        });
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
