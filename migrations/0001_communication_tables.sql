CREATE TABLE conversations (
    id               TEXT        PRIMARY KEY,
    first_principal  TEXT        NOT NULL,
    second_principal TEXT        NOT NULL,
    about_order      TEXT,
    started_at       TIMESTAMPTZ NOT NULL,
    last_message_at  TIMESTAMPTZ NOT NULL,

    CONSTRAINT conversation_has_two_parties CHECK (first_principal <> second_principal)
);

CREATE UNIQUE INDEX conversations_one_per_pair_and_order
    ON conversations (
        LEAST(first_principal, second_principal),
        GREATEST(first_principal, second_principal),
        COALESCE(about_order, '')
    );

CREATE INDEX conversations_by_first_party
    ON conversations (first_principal, last_message_at DESC);
CREATE INDEX conversations_by_second_party
    ON conversations (second_principal, last_message_at DESC);

CREATE TABLE messages (
    id              TEXT        PRIMARY KEY,
    conversation_id TEXT        NOT NULL REFERENCES conversations (id),
    sender          TEXT        NOT NULL,
    body            TEXT        NOT NULL,
    sent_at         TIMESTAMPTZ NOT NULL,

    CONSTRAINT message_is_not_empty CHECK (length(btrim(body)) > 0),
    CONSTRAINT message_is_not_too_long CHECK (length(body) <= 4000)
);

CREATE INDEX messages_in_conversation_newest_first
    ON messages (conversation_id, sent_at DESC, id DESC);
