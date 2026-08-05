#ifndef REPLIES_HPP
#define REPLIES_HPP

namespace Numeric
{
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