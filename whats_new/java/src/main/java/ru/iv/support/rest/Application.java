package ru.iv.support.rest;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.EnableAutoConfiguration;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.boot.context.properties.EnableConfigurationProperties;
import org.springframework.context.annotation.ComponentScan;
import org.springframework.web.socket.config.annotation.EnableWebSocket;

import java.sql.SQLException;

@EnableAutoConfiguration
@EnableWebSocket
@SpringBootApplication
@EnableConfigurationProperties
@ComponentScan({
        "ru.iv.support.rest",
        "ru.iv.support.rest.service",
})
public class Application {
    public static void main(String[] args) throws SQLException {
        SpringApplication.run(Application.class, args);
    }
}
