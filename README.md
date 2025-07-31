# intel7_review_study

## 주차별 과제

[1주차](1st_week/1st_week.md)

## 규칙
1. 소스코드 파일 명 끝에 본인 이니셜 작성 ex) `xxx_jty.c`
2. 작업은 본인 브랜치에서
3. 원본 코드 분석, 공부한 내용, 작동 플로우 등 마크다운 문서(`.md`)로 기록 ex) `xxx_jty.c.md`
   
   [마크다운 작성법](https://namu.wiki/w/%EB%A7%88%ED%81%AC%EB%8B%A4%EC%9A%B4)

   잘 정리해두면 나중에 깃허브로 포폴 만들때 편할듯

4. 뭐 꼬인거 같아서 gpt한테 물어봤더니 `--force` 들어간 명령어 알려주면 절대 하지말것

## github 사용법
### 1. 저장소 클론
```bash
git clone https://github.com/jeong7231/intel7_review_study.git

cd intel7_review_study
```

### 2. 개인 브랜치 생성
브랜치 명은 깃허브 닉네임으로 설정
```bash
git checkout -b 깃허브닉네임
# git checkout -b jeong7231

# 이후 본인 브랜치로 checkout 할때
git checkout 깃허브닉네임
```

### 3. 작업 후 커밋 및 푸시
```bash
# !! 본인 브랜치인지 확인할것 !!
git branch
# 옆에 뜨긴함

git status
# 변경 내용 있을 시 빨간색으로 표시됨

git add .
# add: 변경된 파일을 스테이징 영역에 올림
# . 만 찍으면 전체파일
# 이후 git status시 초록색으로 표시됨

git add 변경파일(추가파일)
# 변경 파일만 add 가능

git commit -m "작업 내용" 
# commit: 작업 내용을 로컬 커밋
# " " 사이 반드시 채워넣을것
# git commit -m "~~~ 수정"

git push -u origin 깃허브닉네임
#  push -u: 원격 저장소(origin)에 브랜치를 업로드
# -u는 이 브랜치와 원격 브랜치를 추적 관계로 설정
# 처음한번만 -u 붙이면 됨 한번 설정되면
# git push 만해줘됨
# git push origin jeong7231 해도됨
```

### 4. 본인 브랜치 최신화
매주 공지, 과제, 변경사항 등
```bash
git checkout 깃허브닉네임

git pull origin main
```




