package ru.iv.support.service;

import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.ControllerAdvice;
import org.springframework.web.bind.annotation.ExceptionHandler;
import org.springframework.web.bind.annotation.ResponseBody;
import org.springframework.web.bind.annotation.ResponseStatus;
import org.springframework.web.servlet.NoHandlerFoundException;

import javax.servlet.http.HttpServletRequest;

@SuppressWarnings("unused")
@ControllerAdvice
final class ExceptionHandlerController {
    @ExceptionHandler(NoHandlerFoundException.class)
    @ResponseStatus(value = HttpStatus.NOT_FOUND)
    @ResponseBody
    public Error requestHandlingNoHandlerFound() {
        return new Error(HttpStatus.NOT_FOUND);
    }

    @ExceptionHandler(Throwable.class)
    @ResponseBody
    ResponseEntity<Error> handleControllerException(HttpServletRequest req, Throwable ex) {
        return new ResponseEntity<>(new Error(HttpStatus.INTERNAL_SERVER_ERROR), HttpStatus.INTERNAL_SERVER_ERROR);
    }

    private final static class Error {
        final String code;
        final String message;

        Error(String code, String message) {
            this.code = code;
            this.message = message;
        }

        Error(HttpStatus status) {
            this(String.valueOf(status.value()), status.getReasonPhrase());
        }
    }
}
