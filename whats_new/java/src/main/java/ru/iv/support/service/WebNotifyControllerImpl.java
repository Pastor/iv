package ru.iv.support.service;

import com.google.common.collect.Sets;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;
import org.springframework.web.socket.CloseStatus;
import org.springframework.web.socket.TextMessage;
import org.springframework.web.socket.WebSocketSession;
import org.springframework.web.socket.handler.TextWebSocketHandler;
import ru.iv.support.notify.Notify;
import ru.iv.support.notify.WebNotifyController;

import java.io.IOException;
import java.util.Set;
import java.util.concurrent.atomic.AtomicBoolean;

@SuppressWarnings("unused")
@Service
@Slf4j
final class WebNotifyControllerImpl extends TextWebSocketHandler implements WebNotifyController {
    private final AtomicBoolean onNotifyPacket = new AtomicBoolean(false);
    private final Set<WebSocketSession> sessions = Sets.newConcurrentHashSet();

    @Override
    protected void handleTextMessage(WebSocketSession session, TextMessage message) throws Exception {
        log.debug(message.getPayload());
    }

    @Override
    public void afterConnectionEstablished(WebSocketSession session) throws Exception {
        log.debug("Connected {}", session.getId());
        sessions.add(session);
    }

    @Override
    public void afterConnectionClosed(WebSocketSession session, CloseStatus status) throws Exception {
        log.debug("Closed {}", session.getId());
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
                    log.error("", e);
                }
            } else {
                sessions.remove(session);
            }
        }
    }

    @Override
    public void notify(Notify notify) {
        if (notify.type == Notify.Type.DEVICE_RECEIVE) {
            if (onNotifyPacket.get())
                broadcast(notify.toString());
        } else {
            broadcast(notify.toString());
        }
    }

    @Override
    public void setNotifyPacket(boolean value) {
        onNotifyPacket.set(value);
    }
}
