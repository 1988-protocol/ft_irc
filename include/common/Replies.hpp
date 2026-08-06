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

    // Channel RPL Replies
    const int RPL_CHANNELMODEIS     = 324;  // 채널 모드를 표시합니다.
    const int RPL_NOTOPIC           = 331;  // 채널에 토픽이 없습니다.
    const int RPL_TOPIC             = 332;  // 채널 토픽을 표시합니다.
    const int RPL_JOIN              = 333;  // 채널에 조인했음을 알립니다.
    const int RPL_PART              = 334;  // 채널에서 파트했음을 알립니다.
    const int RPL_NAMREPLY          = 353;  // 채널 유저 목록을 표시합니다.
    const int RPL_ENDOFNAMES        = 366;  // 채널 유저 목록의 끝을 알립니다.

    // Channel ERR Replies
    const int ERR_NOSUCHCHANNEL         = 403;  // 해당 채널이 존재하지 않습니다.
    const int ERR_TOOMANYCHANNELS       = 405;  // 참여할 수 있는 최대 채널 수를 초과했습니다.
    const int ERR_USERNOTINCHANNEL      = 441;  // 해당 유저가 채널에 없습니다.
    const int ERR_NOTONCHANNEL          = 442;  // 해당 채널의 멤버가 아닙니다.
    const int ERR_USERONCHANNEL         = 443;  // 이미 채널에 있는 유저입니다.
    const int ERR_NEEDMOREPARAMS        = 461;  // 페러미터가 부족합니다.
    const int ERR_CHANNELISFULL         = 471;  // 채널이 가득 찼습니다.
    const int ERR_UNKNOWNMODE           = 472;  // 알 수 없는 모드입니다.
    const int ERR_INVITEONLYCHAN        = 473;  // 초대 전용 채널입니다.
    const int ERR_BADCHANNELKEY         = 475;  // 잘못된 채널 키입니다.
    const int ERR_CHANOPRIVSNEEDED      = 482;  // 채널 운영자만 토픽을 변경할 수 있습니다.
}

#endif
