#ifndef REPLIES_HPP
#define REPLIES_HPP

// RFC 1459 2.4절 numeric reply 코드 중 Parser의 Phase1 범위(PASS/NICK/USER 등록 시퀀스와
// 디스패처 실패 경로)에서 실제로 필요한 것만 정의한다. 001~004 서버 배너, 409(ERR_NOORIGIN) 등은
// 지금 당장 쓰이지 않으므로 추측성으로 미리 추가하지 않는다 — 필요해지는 시점에 추가한다.
namespace Numeric
{
    const int RPL_WELCOME          = 1;    // 등록 완료(PASS+NICK+USER 모두 성공) 시 1회 전송
    const int ERR_UNKNOWNCOMMAND   = 421;  // 디스패처가 map에서 커맨드를 못 찾았을 때
    const int ERR_NONICKNAMEGIVEN  = 431;  // NICK 인자 없음
    const int ERR_ERRONEUSNICKNAME = 432;  // NICK 형식 위반(허용 문자 외)
    const int ERR_NICKNAMEINUSE    = 433;  // NICK 중복
    const int ERR_NOTREGISTERED    = 451;  // 미등록 상태에서 등록 계열 외 커맨드 사용
    const int ERR_NEEDMOREPARAMS   = 461;  // PASS/USER 등 파라미터 부족
    const int ERR_ALREADYREGISTRED = 462;  // 이미 등록된 상태에서 PASS/USER 재시도
    const int ERR_PASSWDMISMATCH   = 464;  // PASS 값이 서버 비밀번호와 불일치
}

#endif
