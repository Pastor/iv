package ru.iv.support.entity;

import lombok.Data;
import lombok.EqualsAndHashCode;
import lombok.RequiredArgsConstructor;
import org.hibernate.validator.constraints.NotEmpty;

import javax.persistence.*;
import javax.validation.constraints.NotNull;

@Entity
@Table(name = "QuestionChoice")
@Data
@EqualsAndHashCode(callSuper = true)
@RequiredArgsConstructor
public final class QuestionChoice extends AbstractEntity {
    @NotEmpty
    @Column(name = "text", nullable = false)
    private String text;

    @NotNull
    @Column(name = "show_order", nullable = false)
    private Integer order;

    @NotEmpty
    @Column(name = "device_enter", nullable = false)
    private String deviceEnter;

    @NotEmpty
    @Column(name = "show_enter", nullable = false)
    private String showEnter;

    @NotNull
    @PrimaryKeyJoinColumn(name = "question_id", referencedColumnName = "id")
    @ManyToOne(fetch = FetchType.LAZY, cascade = CascadeType.REMOVE)
    private Question question;
}
