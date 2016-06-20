package ru.iv.support.entity;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import lombok.*;
import org.springframework.format.annotation.DateTimeFormat;

import javax.persistence.*;
import javax.validation.constraints.NotNull;
import java.time.LocalDateTime;
import java.util.Set;

@Entity
@Table(name = "QuestionResult")
@Data
@EqualsAndHashCode(callSuper = true, exclude = {"answers"})
@RequiredArgsConstructor
public final class QuestionResult extends AbstractEntity {
    public static final String DEFAULT_NAME = "default";

    @NotNull
    @PrimaryKeyJoinColumn(name = "question_id", referencedColumnName = "id")
    @ManyToOne(fetch = FetchType.LAZY, cascade = CascadeType.REMOVE)
    private Question question;

    @JsonIgnore
    @Setter(AccessLevel.NONE)
    @OneToMany(fetch = FetchType.LAZY, mappedBy = "questionResult", cascade = CascadeType.ALL, orphanRemoval = true)
    @OrderBy("id")
    private Set<Answer> answers;

    @Column(name = "name")
    private String name;

    @Column(name = "active", nullable = false)
    private boolean activate;


    @DateTimeFormat(iso = DateTimeFormat.ISO.DATE_TIME)
    @JsonProperty("started_at")
    @JsonSerialize(using = JsonLocalDateTimeSerializer.class)
    @Column(name = "started_at")
    private LocalDateTime startedAt;

    @DateTimeFormat(iso = DateTimeFormat.ISO.DATE_TIME)
    @JsonProperty("stopped_at")
    @JsonSerialize(using = JsonLocalDateTimeSerializer.class)
    @Column(name = "stopped_at")
    private LocalDateTime stoppedAt;
}
