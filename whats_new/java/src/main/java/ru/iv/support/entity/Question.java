package ru.iv.support.entity;

import com.fasterxml.jackson.annotation.JsonIgnore;
import lombok.*;
import org.hibernate.validator.constraints.NotEmpty;

import javax.persistence.*;
import javax.validation.constraints.NotNull;
import javax.validation.constraints.Size;
import java.util.Set;

@Entity
@Table(name = "Question")
@Data
@EqualsAndHashCode(callSuper = true, exclude = {"results", "choices"})
@RequiredArgsConstructor
public final class Question extends AbstractEntity {
    @NotEmpty
    @Column(name = "text", nullable = false)
    private String text;

    @Column(name = "time_count")
    private Long timeCount;

    @JsonIgnore
    @NotNull
    @PrimaryKeyJoinColumn(name = "question_sequence_id", referencedColumnName = "id")
    @ManyToOne(fetch = FetchType.LAZY, cascade = CascadeType.REMOVE)
    private QuestionSequence sequence;

    @Setter(AccessLevel.NONE)
    @OneToMany(fetch = FetchType.LAZY, mappedBy = "question", cascade = CascadeType.ALL, orphanRemoval = true)
    @OrderBy("id")
    private Set<QuestionResult> results;

    @Size(min = 1, message = "Ответы на вопрос")
    @Setter(AccessLevel.NONE)
    @OneToMany(fetch = FetchType.LAZY, mappedBy = "question", cascade = CascadeType.ALL, orphanRemoval = true)
    @OrderBy("id")
    private Set<QuestionChoice> choices;
}
