#pragma once

#include "Iop_FileIo.h"

namespace Iop
{
	class CFileIoHandler2300 : public CFileIo::CHandler
	{
	public:
						CFileIoHandler2300(CIoman*, CSifMan&);

		virtual void	Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*) override;

	private:
		struct COMMANDHEADER
		{
			uint32			semaphoreId;
			uint32			resultPtr;
			uint32			resultSize;
		};

		struct OPENCOMMAND
		{
			COMMANDHEADER	header;
			uint32			flags;
			uint32			somePtr1;
			char			fileName[256];
		};

		struct CLOSECOMMAND
		{
			COMMANDHEADER	header;
			uint32			fd;
		};

		struct READCOMMAND
		{
			COMMANDHEADER	header;
			uint32			fd;
			uint32			buffer;
			uint32			size;
		};

		struct SEEKCOMMAND
		{
			COMMANDHEADER	header;
			uint32			fd;
			uint32			offset;
			uint32			whence;
		};

		struct ACTIVATECOMMAND
		{
			COMMANDHEADER	header;
			char			device[256];
		};

		struct REPLYHEADER
		{
			uint32			semaphoreId;
			uint32			commandId;
			uint32			resultPtr;
			uint32			resultSize;
		};

		struct OPENREPLY
		{
			REPLYHEADER		header;
			uint32			result;
			uint32			unknown2;
			uint32			unknown3;
			uint32			unknown4;
		};

		struct CLOSEREPLY
		{
			REPLYHEADER		header;
			uint32			result;
			uint32			unknown2;
			uint32			unknown3;
			uint32			unknown4;
		};

		struct READREPLY
		{
			REPLYHEADER		header;
			uint32			result;
			uint32			unknown2;
			uint32			unknown3;
			uint32			unknown4;
		};

		struct SEEKREPLY
		{
			REPLYHEADER		header;
			uint32			result;
			uint32			unknown2;
			uint32			unknown3;
			uint32			unknown4;
		};

		struct ACTIVATEREPLY
		{
			REPLYHEADER		header;
			uint32			result;
			uint32			unknown2;
			uint32			unknown3;
			uint32			unknown4;
		};

		void			CopyHeader(REPLYHEADER&, const COMMANDHEADER&);
		
		uint32			m_resultPtr[2];
		CSifMan&		m_sifMan;
	};
};
