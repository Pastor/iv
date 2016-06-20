package ru.iv.support.notify;

import org.springframework.web.socket.WebSocketHandler;

public interface WebNotifyController extends WebSocketHandler {
    void broadcast(String message);
    void notify(Notify notify);
}
