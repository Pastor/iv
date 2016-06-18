package ru.iv.support;

import org.springframework.web.socket.WebSocketHandler;

public interface WebDeviceController extends WebSocketHandler {
    void broadcast(String message);
}
