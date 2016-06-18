package ru.iv.support.service;

import com.google.common.io.CharStreams;
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

import javax.annotation.PostConstruct;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.URISyntaxException;
import java.net.URL;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;

@RestController("deviceController")
final class RestDeviceController {
    @Autowired
    @Qualifier("defaultTaskExecutor")
    private TaskExecutor executor;

    @Autowired
    private DeviceController controller;

    @RequestMapping(path = "/version", produces = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.GET)
    @ResponseBody
    public String version() throws IOException, URISyntaxException {
        final URL resource = RestDeviceController.class.getResource("/version.json");
        return CharStreams.toString(new InputStreamReader(resource.openStream()));
    }

    @PostConstruct
    private void init() {
        executor.execute(() -> {
            while (true) {
                final Event event = next(controller.events());
                if (event != null) {
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
