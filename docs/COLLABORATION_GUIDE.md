# 🤝 GitHub 협업 가이드 (3인 팀)

> 시니어 엔지니어 관점에서 정리한 실무형 협업 가이드입니다.  
> 처음 GitHub 협업을 시작하는 팀원들이 혼란 없이 작업할 수 있도록 구체적인 예시와 함께 설명합니다.

---

## 📋 목차

1. [브랜치 전략](#1-브랜치-전략)
2. [커밋 컨벤션](#2-커밋-컨벤션)
3. [Pull Request (PR) 컨벤션](#3-pull-request-pr-컨벤션)
4. [Issue 관리](#4-issue-관리)
5. [코드 리뷰 가이드라인](#5-코드-리뷰-가이드라인)
6. [데일리 워크플로우 (실전 루틴)](#6-데일리-워크플로우-실전-루틴)
7. [자주 하는 실수 & 대처법](#7-자주-하는-실수--대처법)

---

## 1. 브랜치 전략

### 브랜치 구조

```
main        ← 배포 가능한 안정 버전 (직접 push 금지)
 └── dev    ← 통합 개발 브랜치 (직접 push 금지)
      ├── feature/기능명     ← 새 기능 개발
      ├── fix/버그명         ← 버그 수정
      └── refactor/대상명   ← 리팩토링
```

### 브랜치 명명 규칙

| 브랜치 타입 | 패턴 | 예시 |
|---|---|---|
| 새 기능 | `feature/기능명` | `feature/client-connect` |
| 버그 수정 | `fix/버그명` | `fix/fd-leak` |
| 리팩토링 | `refactor/대상` | `refactor/send-message` |
| 문서 작업 | `docs/내용` | `docs/readme-update` |

> ⚠️ **절대 규칙**: `main`과 `dev`에는 **직접 push하지 않습니다**.  
> 반드시 본인 작업 브랜치 → PR → merge 순서로 진행합니다.

### 브랜치 생성 순서

```bash
# 1. 항상 최신 dev 기준으로 시작
git checkout dev
git pull origin dev

# 2. 새 브랜치 생성 후 이동
git checkout -b feature/client-connect

# 3. 작업 완료 후 PR 생성
git push origin feature/client-connect
```

---

## 2. 커밋 컨벤션

### 커밋 메시지 형식

```
<type>: <subject>

[body - 선택사항]
```

### type 종류

| type | 설명 |
|---|---|
| `feat` | 새로운 기능 추가 |
| `fix` | 버그 수정 |
| `refactor` | 코드 개선 (기능 변화 없음) |
| `docs` | 문서 작성/수정 |
| `style` | 포맷팅, 세미콜론 등 (기능 변화 없음) |
| `test` | 테스트 코드 추가/수정 |
| `chore` | 빌드, 설정 파일 변경 |

### 커밋 메시지 예시

```
feat: add client connection handler

fix: resolve fd leak on client disconnect

refactor: simplify broadcast logic using helper function

docs: update README with build instructions

chore: add .gitignore for object files
```

### ❓ 커밋에 본인 아이디를 넣어야 할까?

**결론: 넣지 않아도 됩니다.**

`git log`는 이미 커미터(author) 정보를 자동으로 기록합니다.  
아이디를 커밋 메시지에 반복해서 쓰는 건 오히려 노이즈가 됩니다.

```bash
# git log --oneline 예시 → 이미 누가 커밋했는지 알 수 있음
a1b2c3d (borlee) feat: add select-based fd monitoring
d4e5f6g (jooyepar) fix: handle partial message buffer
h7i8j9k (mjoh) docs: add compilation guide
```

단, **특정 작업이 누군가의 요청이나 페어 작업**인 경우 body에 명시할 수 있습니다.

```
feat: add signal handler for SIGPIPE

Co-authored-by: jooyepar <jooyepar@github.com>
```

### git 계정 설정 확인 (필수)

팀 전원이 아래 설정을 완료했는지 확인하세요.

```bash
git config --global user.name "본인 깃허브 아이디"
git config --global user.email "깃허브 계정 이메일"

# 확인
git config --list
```

---

## 3. Pull Request (PR) 컨벤션

### PR 제목 형식

```
[type] 작업 내용을 간결하게
```

예시:
```
[feat] 클라이언트 연결 핸들러 추가
[fix] 소켓 FD 누수 수정
[docs] 협업 가이드 작성
```

### PR 본문 템플릿

PR을 생성할 때 아래 내용을 포함합니다.  
GitHub 레포의 `.github/PULL_REQUEST_TEMPLATE.md`로 자동화할 수 있습니다.

```markdown
## 📌 작업 개요
이 PR에서 무엇을 했는지 1~3줄로 설명합니다.

## 🔗 관련 Issue
closes #이슈번호  (이슈가 있다면)

## ✅ 작업 내용
- [ ] 구현한 기능 1
- [ ] 구현한 기능 2

## 🧪 테스트 방법
어떻게 테스트했는지 간략히 설명합니다.

## 💬 리뷰어에게
리뷰어가 특별히 봐줬으면 하는 부분이나 논의할 사항을 적습니다.
```

### PR 규칙 요약

| 항목 | 규칙 |
|---|---|
| Merge 조건 | **1인 이상 Approve** 필수 |
| Merge 대상 | `feature/*` → `dev` |
| `dev` → `main` | 팀 전원 합의 후 진행 |
| PR 크기 | **하나의 PR = 하나의 목적** (너무 많은 변경 X) |
| Assignee | 본인을 지정 |
| Reviewer | 나머지 팀원 전원 또는 최소 1인 지정 |

> 💡 **실무 팁**: PR은 작을수록 리뷰가 쉽습니다.  
> "일단 다 완성하고 PR"이 아니라, **의미 있는 단위로 자주** 올리는 것이 좋습니다.

---

## 4. Issue 관리

### Issue를 쓰는 이유

- 할 일을 **투명하게 공유**하기 위해
- **PR과 연결**하여 어떤 이슈를 해결했는지 추적하기 위해
- 버그나 아이디어를 **잊지 않기 위해** 기록

### Issue 제목 형식

```
[type] 내용 요약
```

예시:
```
[feat] select() 기반 I/O 멀티플렉싱 구현
[bug] 클라이언트 종료 시 메모리 누수 발생
[question] send() 실패 시 재시도 로직 필요한가?
```

### Issue 본문 템플릿

```markdown
## 설명
이슈 내용을 설명합니다.

## 재현 방법 (버그의 경우)
1. 서버 실행
2. 클라이언트 3개 연결
3. 클라이언트 2 강제 종료 시 서버 크래시

## 기대 동작
정상적으로 나머지 클라이언트에게 "client X has left" 전송

## 담당자
@borlee
```

### Issue & PR 연결

PR 본문에 아래와 같이 작성하면 PR merge 시 Issue가 자동으로 닫힙니다.

```
closes #5
fixes #12
resolves #3
```

### Issue Label 활용

| Label | 의미 |
|---|---|
| `feat` | 새 기능 |
| `bug` | 버그 |
| `docs` | 문서 |
| `discussion` | 논의 필요 |
| `in progress` | 작업 중 |
| `review needed` | 리뷰 요청 |

---

## 5. 코드 리뷰 가이드라인

### 리뷰어의 역할

- **24시간 내** 리뷰 완료를 목표로 합니다
- 단순히 "LGTM(좋아 보임)"만 하지 말고, 실제로 읽고 코멘트를 남깁니다

### 리뷰 코멘트 작성 방법

| 접두어 | 의미 |
|---|---|
| `[필수]` | 반드시 수정해야 merge 가능 |
| `[제안]` | 수정하면 좋지만, 작성자 판단에 맡김 |
| `[질문]` | 이해가 안 되는 부분 질문 |
| `[칭찬]` | 잘 짠 코드에 긍정 피드백 |

예시:
```
[필수] 이 함수에서 fd가 -1일 경우 처리가 없습니다.
[제안] 이 로직은 별도 함수로 분리하면 가독성이 좋을 것 같습니다.
[질문] select() timeout을 NULL로 설정한 이유가 있나요?
[칭찬] 에러 메시지를 stderr로 출력한 처리가 깔끔합니다 👍
```

### PR 작성자의 역할

- 리뷰 코멘트에 **반드시 답변**합니다 (무시 금지)
- 수정 완료 후 `resolve` 버튼을 눌러 완료 처리합니다
- 큰 변경이 필요하면 `Re-request review`로 재리뷰를 요청합니다

---

## 6. 데일리 워크플로우 (실전 루틴)

### 매일 작업 시작 시

```bash
# 1. 최신 dev 반영
git checkout dev
git pull origin dev

# 2. 내 작업 브랜치로 이동 (이미 있는 경우)
git checkout feature/my-feature

# 3. dev 변경사항을 내 브랜치에 반영 (충돌 방지)
git merge dev
# 또는 git rebase dev (히스토리를 깔끔하게 유지하고 싶을 때)
```

### 작업 완료 후 push

```bash
git add .
git commit -m "feat: implement message broadcast"
git push origin feature/my-feature
```

### PR merge 후

```bash
# 작업이 끝난 브랜치는 삭제하여 브랜치 목록을 깔끔하게 유지
git branch -d feature/my-feature
git push origin --delete feature/my-feature
```

### 전체 흐름 요약

```
Issue 생성
   ↓
dev 최신화 후 feature 브랜치 생성
   ↓
작업 & 커밋
   ↓
PR 생성 (feature → dev)
   ↓
팀원 1인 이상 리뷰 & Approve
   ↓
Merge & 브랜치 삭제
   ↓
(기능 완성 시) dev → main PR 생성 & 전원 Approve
```

---

## 7. 자주 하는 실수 & 대처법

### ❌ main/dev에 직접 push했을 때

```bash
# push한 커밋을 되돌려야 합니다 → 팀원에게 즉시 알리세요!
# 혼자 해결하려 하지 말고 공유가 우선입니다.
```

### ❌ 충돌(Conflict)이 발생했을 때

```bash
# merge 또는 pull 중 충돌 발생 시
git status                  # 충돌 파일 확인
# 해당 파일을 열어 <<<<<<, =======, >>>>>>> 표시를 찾아 수동으로 해결
git add 해결한_파일
git commit -m "fix: resolve merge conflict"
```

> 충돌이 무섭다고 피하면 안 됩니다.  
> **자주 pull하고 자주 merge할수록 충돌은 작아집니다.**

### ❌ 커밋 메시지를 잘못 작성했을 때

```bash
# 가장 최근 커밋 메시지 수정 (push 전에만!)
git commit --amend -m "fix: 올바른 메시지"

# push 이후에는 절대 force push하지 마세요 → 팀원에게 영향 줍니다
```

### ❌ 잘못된 파일을 add/commit했을 때

```bash
# add 취소 (commit 전)
git restore --staged 파일명

# commit 취소 (push 전, 변경사항은 유지)
git reset --soft HEAD~1
```

---

## 📎 요약 체크리스트

작업 시작 전:
- [ ] `git pull origin dev`로 최신화 완료
- [ ] `dev` 기반으로 새 브랜치 생성

커밋 전:
- [ ] 커밋 단위가 하나의 의미 있는 변경인가?
- [ ] 커밋 메시지가 컨벤션에 맞는가?

PR 생성 시:
- [ ] PR 제목이 `[type] 설명` 형식인가?
- [ ] 관련 Issue 번호 연결 (`closes #N`)
- [ ] Assignee/Reviewer 지정 완료
- [ ] PR 본문에 작업 내용 및 테스트 방법 작성

Merge 후:
- [ ] 작업 완료된 브랜치 삭제

---

> 처음엔 이 과정이 번거롭게 느껴질 수 있습니다.  
> 하지만 협업에서 발생하는 대부분의 혼란(누가 뭘 했는지 모름, 충돌, 누락)은  
> **이 가이드를 지키는 것만으로 80% 이상 예방**됩니다.  
> 팀원 모두 화이팅! 💪
