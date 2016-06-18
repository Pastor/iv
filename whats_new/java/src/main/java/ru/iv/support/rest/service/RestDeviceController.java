package ru.iv.support.rest.service;

import com.google.common.io.CharStreams;
import org.springframework.http.MediaType;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;
import org.springframework.web.bind.annotation.ResponseBody;
import org.springframework.web.bind.annotation.RestController;

import java.io.IOException;
import java.io.InputStreamReader;
import java.net.URISyntaxException;
import java.net.URL;

@RestController("deviceController")
final class RestDeviceController {

    @RequestMapping(path = "/version", produces = MediaType.APPLICATION_JSON_UTF8_VALUE, method = RequestMethod.GET)
    @ResponseBody
    public String version() throws IOException, URISyntaxException {
        final URL resource = RestDeviceController.class.getResource("/version.json");
        return CharStreams.toString(new InputStreamReader(resource.openStream()));
    }
}
