package ru.iv.support;

import org.springframework.web.socket.WebSocketHandler;

public interface WebNotifyController extends WebSocketHandler {
    void broadcast(String message);
}
