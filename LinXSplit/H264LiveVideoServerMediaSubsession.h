


#include ".\live555\H264VideoRTPSink.hh"
#include ".\live555\ByteStreamFileSource.hh"
#include ".\live555\H264VideoStreamFramer.hh"
#include ".\live555\FileServerMediaSubsession.hh"

class H264LiveVideoServerMediaSubsession : public OnDemandServerMediaSubsession{
public:
	static H264LiveVideoServerMediaSubsession*
		createNew(UsageEnvironment& env, Boolean reuseFirstSource);

private:
	H264LiveVideoServerMediaSubsession(UsageEnvironment& env,
		Boolean reuseFirstSource);
	// called only by createNew();
	virtual ~H264LiveVideoServerMediaSubsession();

private:
	// redefined virtual functions
	virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
		unsigned& estBitrate);
	virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
		unsigned char rtpPayloadTypeIfDynamic,
		FramedSource* inputSource);
protected:
	virtual char const* sdpLines();
};