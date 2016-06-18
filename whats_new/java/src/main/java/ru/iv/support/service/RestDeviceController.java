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
import ru.iv.support.Event;
import ru.iv.support.WebDeviceController;

import javax.annotation.PostConstruct;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;

@SuppressWarnings("unused")
@RestController("deviceController")
final class RestDeviceController {
    private static final Gson GSON = new GsonBuilder().setPrettyPrinting().create();
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

    @RequestMapping(path = "/", method = RequestMethod.GET, produces = MediaType.TEXT_HTML_VALUE)
    public String index() throws IOException {
        final URL resource = RestDeviceController.class.getResource("/public/index.html");
        return CharStreams.toString(new InputStreamReader(resource.openStream()));
    }

    @PostConstruct
    private void init() {
        executor.execute(() -> {
            while (true) {
                final Event event = next(deviceController.events());
                if (event != null) {
                    final String message = GSON.toJson(event, Event.class);
                    webController.broadcast(message);
                    System.out.println(event);
                }
            }
        });
    }

    private static Event next(BlockingQueue<Event> events) {
        try {
            return events.poll(10000, TimeUnit.MILLISECONDS);
        } catch (Exception ex) {
            return null;
        }
    }
}
