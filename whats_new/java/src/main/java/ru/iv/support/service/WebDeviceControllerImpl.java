package ru.iv.support.service;

import com.google.common.collect.Sets;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.web.socket.CloseStatus;
import org.springframework.web.socket.TextMessage;
import org.springframework.web.socket.WebSocketSession;
import org.springframework.web.socket.handler.TextWebSocketHandler;
import ru.iv.support.WebDeviceController;

import java.io.IOException;
import java.util.Set;

public final class WebDeviceControllerImpl extends TextWebSocketHandler implements WebDeviceController {
    private static final Logger logger = LoggerFactory.getLogger(WebDeviceControllerImpl.class);

    private final Set<WebSocketSession> sessions = Sets.newConcurrentHashSet();

    @Override
    protected void handleTextMessage(WebSocketSession session, TextMessage message) throws Exception {
        logger.debug(message.getPayload());
    }

    @Override
    public void afterConnectionEstablished(WebSocketSession session) throws Exception {
        logger.debug("Connected {}", session.getId());
        sessions.add(session);
    }

    @Override
    public void afterConnectionClosed(WebSocketSession session, CloseStatus status) throws Exception {
        logger.debug("Closed {}", session.getId());
        sessions.remove(session);
    }

    @Override
    public void broadcast(String message) {
        final TextMessage payload = new TextMessage(message);
        for (WebSocketSession session : sessions) {
            if (session.isOpen()) {
                try {
                    session.sendMessage(payload);
                } catch (IOException e) {
                    logger.error("", e);
                }
            } else {
                sessions.remove(session);
            }
        }
    }
}
