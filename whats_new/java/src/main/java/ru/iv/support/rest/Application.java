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
import org.springframework.web.socket.config.annotation.EnableWebSocket;

import java.sql.SQLException;

@EnableAutoConfiguration
@EnableWebSocket
@SpringBootApplication
@EnableAsync
@EnableConfigurationProperties
@ComponentScan({
        "ru.iv.support.rest",
        "ru.iv.support.service",
        "ru.iv.support.dll",
})
public class Application {

    @Bean(name = "defaultTaskExecutor")
    public TaskExecutor getTaskExecutor() {
        return new SimpleAsyncTaskExecutor();
    }

    public static void main(String[] args) throws SQLException {
        SpringApplication.run(Application.class, args);
    }
}
