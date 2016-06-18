package ru.iv.support.rest;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.EnableAutoConfiguration;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.context.properties.EnableConfigurationProperties;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.ComponentScan;
import org.springframework.core.task.SimpleAsyncTaskExecutor;
import org.springframework.core.task.TaskExecutor;
import org.springframework.scheduling.annotation.EnableAsync;
import org.springframework.web.servlet.config.annotation.EnableWebMvc;
import org.springframework.web.socket.config.annotation.EnableWebSocket;
import org.springframework.web.socket.config.annotation.WebSocketConfigurer;
import org.springframework.web.socket.config.annotation.WebSocketHandlerRegistry;
import org.springframework.web.socket.server.support.DefaultHandshakeHandler;
import ru.iv.support.WebDeviceController;
import ru.iv.support.service.WebDeviceControllerImpl;

import java.sql.SQLException;

@EnableAutoConfiguration
@EnableWebSocket
@EnableWebMvc
@SpringBootApplication
@EnableAsync
@EnableConfigurationProperties
@ComponentScan({
        "ru.iv.support.rest",
        "ru.iv.support.service",
        "ru.iv.support.dll",
})
public class Application implements WebSocketConfigurer {

    @Bean(name = "defaultTaskExecutor")
    public TaskExecutor getTaskExecutor() {
        return new SimpleAsyncTaskExecutor();
    }

    public static void main(String[] args) throws SQLException {
        SpringApplication.run(Application.class, args);
    }

    @Bean(name = "webController")
    public WebDeviceController webController() {
        return new WebDeviceControllerImpl();
    }

    @Override
    public void registerWebSocketHandlers(WebSocketHandlerRegistry registry) {
        registry.addHandler(webController(), "/ws").setHandshakeHandler(new DefaultHandshakeHandler());
    }
}
