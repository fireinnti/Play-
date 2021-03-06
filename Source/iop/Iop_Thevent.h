#ifndef _IOP_THEVENT_H_
#define _IOP_THEVENT_H_

#include "Iop_Module.h"

class CIopBios;

namespace Iop
{
	class CThevent : public CModule
	{
	public:
						CThevent(CIopBios&, uint8*);
		virtual			~CThevent();

		std::string		GetId() const;
		std::string		GetFunctionName(unsigned int) const;
		void			Invoke(CMIPS&, unsigned int);

	private:
		struct EVENT
		{
			uint32		attributes;
			uint32		options;
			uint32		initValue;
		};

		uint32			CreateEventFlag(EVENT*);
		uint32			SetEventFlag(uint32, uint32);
		uint32			iSetEventFlag(uint32, uint32);
		uint32			ClearEventFlag(uint32, uint32);
		uint32			WaitEventFlag(uint32, uint32, uint32, uint32);
		uint32			ReferEventFlagStatus(uint32, uint32);

		uint8*			m_ram;
		CIopBios&		m_bios;
	};
}

#endif
