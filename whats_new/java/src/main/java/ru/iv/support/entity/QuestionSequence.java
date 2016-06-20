package ru.iv.support.entity;

import lombok.*;
import org.hibernate.validator.constraints.NotEmpty;

import javax.persistence.*;
import java.util.List;

@Entity
@Table(name = "QuestionSequence")
@Data
@EqualsAndHashCode(callSuper = true, exclude = {"questions"})
@RequiredArgsConstructor
public final class QuestionSequence extends AbstractEntity {

    @NotEmpty
    @Column(name = "name", nullable = false)
    private String name;

    @Setter(AccessLevel.NONE)
    @OneToMany(fetch = FetchType.LAZY, mappedBy = "sequence", cascade = CascadeType.ALL, orphanRemoval = true)
    @OrderBy("id")
    private List<Question> questions;
}
