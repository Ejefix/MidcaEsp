# MIDCA
## Target Platform / Платформа

- MCU: ESP32
- IDE: Visual Studio Code
- Framework: PlatformIO

> ⚠️ Project status: core system is under active development and not yet stabilized.

MIDCA is an automation system built around the Intent model.

The core idea of the project is that neither the user, nor sensors, nor scenarios, nor servers directly control devices. Every action in the system is first converted into an Intent and only then can it affect the state of the world.


---

# Architecture Rules

These rules form the foundation of the system and cannot be violated without changing the core architecture of the project.

---

## Rule #1. Everything is an Intent

Every event in the system is represented as an Intent.

It does not matter who created the Intent:

- user;
- scenario;
- sensor;
- system module;
- external device;
- server.

All participants in the system create the same object — Intent.

**An Intent does not belong to its creator. An Intent belongs to its recipient.**

---

## Rule #2. Creator and resource owner are different entities

Creating an Intent does not grant the right to control a resource.

Each resource has a single owner.

Only the resource owner has the right to:

- change its state;
- set locks;
- accept an Intent;
- reject an Intent;
- define the reason for rejection.

No other entity can make these decisions.

---

## Rule #3. An Intent is addressed to a target

An Intent is always created for a specific target.

It does not matter who created the Intent.

What matters is which resource it is intended for.

The Intent contains a destination address and is passed to the resource owner for decision-making.

---

## Rule #4. Only the arbitrator makes decisions

The creator of an Intent does not decide whether it is executed.

The executor of an Intent does not decide whether it is executed.

Only the arbitrator determines the fate of an Intent:

- execute;
- defer;
- stop;
- reject.

---

## Rule #5. Every decision must be explainable

The system does not allow unexplained decisions.

Any state change of an Intent must include a reason.

If an Intent is not executed, the system must store the reason for the rejection.

The reason may include:

- execution result;
- arbitration rule;
- resource lock;
- another Intent.

---

## Rule #6. The world is changed only through Intents

Neither sensors, nor scenarios, nor servers, nor users directly change the system state.

They only create Intents.

A change in the world state is possible only after:

1. The Intent is created.
2. The Intent passes arbitration.
3. The resource owner accepts the decision.
4. The executor successfully applies the change.

Only after this is the world state considered changed.

---

# Project Philosophy

The system has no direct device control.

There is only a stream of Intents processed according to system rules.

The outcome is not determined by the source of the event.

The outcome is determined by the resource owner and arbitration rules.

That is why every state change can be explained, reproduced, and analyzed.

# MIDCA RU
> ⚠️ Статус проекта: ядро системы находится в активной разработке и ещё не стабилизировано.

MIDCA — система автоматизации, построенная вокруг модели намерений (Intent).

Основная идея проекта заключается в том, что ни пользователь, ни датчик, ни сценарий, ни сервер не управляют устройствами напрямую. Любое действие в системе сначала превращается в Intent и только после этого может повлиять на состояние мира.

---

# Законы архитектуры

Эти законы являются фундаментом системы и не могут быть нарушены без изменения самой архитектуры проекта.

---

## Закон №1. Всё является Intent

Любое событие в системе представляется как Intent.

Не имеет значения, кто создал Intent:

- пользователь;
- сценарий;
- датчик;
- системный модуль;
- внешнее устройство;
- сервер.

Все участники системы создают один и тот же объект — Intent.

**Intent не принадлежит создателю. Intent принадлежит получателю.**

---

## Закон №2. Создатель и владелец ресурса — разные сущности

Создание Intent не даёт права управлять ресурсом.

Каждый ресурс имеет единственного владельца.

Только владелец ресурса имеет право:

- изменять своё состояние;
- устанавливать блокировки;
- принимать Intent;
- отклонять Intent;
- определять причину отклонения.

Никто другой не может принимать эти решения.

---

## Закон №3. Intent адресуется цели

Intent всегда создаётся для конкретной цели.

Не имеет значения, кто создал Intent.

Важно только то, для какого ресурса он предназначен.

Intent содержит адрес назначения и передаётся владельцу ресурса для принятия решения.

---

## Закон №4. Решение принимает только арбитр

Создатель Intent не принимает решение о его выполнении.

Исполнитель Intent также не принимает решение о его выполнении.

Только арбитр определяет дальнейшую судьбу Intent:

- выполнить;
- отложить;
- остановить;
- отклонить.

---

## Закон №5. Каждое решение должно быть объяснимо

Система не допускает необъяснимых решений.

Любое изменение состояния Intent должно иметь причину.

Если Intent не был выполнен, система обязана сохранить информацию о причине отказа.

Причиной может быть:

- результат выполнения;
- правило арбитража;
- блокировка ресурса;
- другой Intent.

---

## Закон №6. Мир изменяется только через Intent

Ни датчик, ни сценарий, ни сервер, ни пользователь не изменяют состояние системы напрямую.

Они лишь создают Intent.

Изменение состояния мира возможно только после того, как:

1. Intent создан.
2. Intent прошёл арбитраж.
3. Владелец ресурса принял решение.
4. Исполнитель успешно применил изменение.

Только после этого состояние мира считается изменённым.

---

# Философия проекта

В системе отсутствует прямое управление устройствами.

Существует только поток Intent, который проходит через правила системы.

Не источник события определяет результат.

Результат определяется владельцем ресурса и правилами арбитража.

Именно поэтому любое изменение состояния может быть объяснено, воспроизведено и проанализировано.