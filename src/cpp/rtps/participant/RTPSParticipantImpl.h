/*************************************************************************
 * Copyright (c) 2014 eProsima. All rights reserved.
 *
 * This copy of eProsima Fast RTPS is licensed to you under the terms described in the
 * FASTRTPS_LIBRARY_LICENSE file included in this distribution.
 *
 *************************************************************************/

/**
 * @file RTPSParticipant.h
 */

#ifndef RTPSParticipantIMPL_H_
#define RTPSParticipantIMPL_H_
#ifndef DOXYGEN_SHOULD_SKIP_THIS_PUBLIC
#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <sys/types.h>

#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif


namespace boost
{
namespace interprocess{class interprocess_semaphore;}
namespace asio{class io_service;}
class recursive_mutex;
}

#include <fastrtps/rtps/attributes/RTPSParticipantAttributes.h>
#include <fastrtps/rtps/common/Guid.h>

//Santi - Adding .h files for the new transport layers
#include <fastrtps/rtps/network/NetworkFactory.h>
#include <fastrtps/rtps/network/ReceiverResource.h>
#include <fastrtps/rtps/network/SenderResource.h>
#include <fastrtps/rtps/messages/MessageReceiver.h>

namespace eprosima {
namespace fastrtps{

	class WriterQos;
	class ReaderQos;
	class TopicAttributes;
	class MessageReceiver;

namespace rtps {

class RTPSParticipant;
class RTPSParticipantListener;
class ListenResource;
class ResourceSend;
class ResourceEvent;
class BuiltinProtocols;
struct CDRMessage_t;
class Endpoint;
class RTPSWriter;
class WriterAttributes;
class WriterHistory;
class WriterListener;
class RTPSReader;
class ReaderAttributes;
class ReaderHistory;
class ReaderListener;

/*
	Receiver Control block is a struct we use to encapsulate the resources that take part in message reception.
	It contains:
	-A ReceiverResource (as produced by the NetworkFactory Element)
	-A list of associated wirters and readers
	-A buffer for message storage
	-A mutex for the lists
	The idea is to create the thread that performs blocking calllto ReceiverResource.Receive and processes the message
	from the Receiver, so the Transport Layer does not need to be aware of the existence of what is using it.

*/
typedef struct ReceiverControlBlock{
	ReceiverResource Receiver;
	MessageReceiver* mp_receiver;		//Associated Readers/Writers inside of MessageReceiver
	boost::mutex mtx; //Fix declaration
	boost::thread* m_thread;
   bool resourceAlive;
	ReceiverControlBlock(ReceiverResource&& rec):m_thread(nullptr),Receiver(std::move(rec)), mp_receiver(nullptr), resourceAlive(true)
	{
	}
	ReceiverControlBlock(ReceiverControlBlock&& origen):m_thread(origen.m_thread),Receiver(std::move(origen.Receiver)), mp_receiver(origen.mp_receiver),resourceAlive(true)
	{
	   origen.m_thread = nullptr;
      origen.mp_receiver = nullptr;
	}
   
   private:
   ReceiverControlBlock(const ReceiverControlBlock&) = delete;
   const ReceiverControlBlock& operator=(const ReceiverControlBlock&) = delete;

} ReceiverControlBlock;


/**
 * @brief Class RTPSParticipantImpl, it contains the private implementation of the RTPSParticipant functions and allows the creation and removal of writers and readers. It manages the send and receive threads.
 * @ingroup RTPS_MODULE
 */
class RTPSParticipantImpl
{
public:
	/**
	* @param param
	* @param guidP
	* @param part
	* @param plisten
	*/
	RTPSParticipantImpl(const RTPSParticipantAttributes &param,
			const GuidPrefix_t& guidP,RTPSParticipant* part,RTPSParticipantListener* plisten= nullptr);
	virtual ~RTPSParticipantImpl();

	/**
	* Get associated GUID
	* @return Associated GUID
	*/
	inline const GUID_t& getGuid() const {return m_guid;};

	//! Announce RTPSParticipantState (force the sending of a DPD message.)
	void announceRTPSParticipantState();
	//!Stop the RTPSParticipant Announcement (used in tests to avoid multiple packets being send)
	void stopRTPSParticipantAnnouncement();
	//!Reset to timer to make periodic RTPSParticipant Announcements.
	void resetRTPSParticipantAnnouncement();

	void loose_next_change();

	/**
	 * Activate a Remote Endpoint defined in the Static Discovery.
	 * @param pguid GUID_t of the endpoint.
	 * @param userDefinedId userDeinfed Id of the endpoint.
	 * @param kind kind of endpoint
	 * @return True if correct.
	 */
	bool newRemoteEndpointDiscovered(const GUID_t& pguid, int16_t userDefinedId,EndpointKind_t kind);

	/**
	 * Assert the liveliness of a remote participant
	 * @param guidP GuidPrefix_t of the participant.
	 */
	void assertRemoteRTPSParticipantLiveliness(const GuidPrefix_t& guidP);

    /**
	 * Get the RTPSParticipant ID
	 * @return RTPSParticipant ID
	 */
    inline uint32_t getRTPSParticipantID() const { return (uint32_t)m_att.participantID;};
    //!Post to the resource semaphore
    void ResourceSemaphorePost();
    //!Wait for the resource semaphore
    void ResourceSemaphoreWait();
    //!Get Pointer to the Event Resource.
	ResourceEvent& getEventResource();
    //!Send Method - Deprecated - Stays here for reference purposes
	void sendSync(CDRMessage_t* msg, Endpoint *pend, const Locator_t& destination_loc);
    //!Get the participant Mutex
    boost::recursive_mutex* getParticipantMutex() const {return mp_mutex;};
	/**
	* Get the participant listener
	* @return participant listener
	*/
    inline RTPSParticipantListener* getListener(){return mp_participantListener;}
	/**
	* Get the participant
	* @return participant
	*/
    inline RTPSParticipant* getUserRTPSParticipant(){return mp_userParticipant;};

    /* Not needed anymore, stays for reference pursposes
	bool assignLocatorForBuiltin_unsafe(LocatorList_t& list, bool isMulti, bool isFixed);*/

private:
	//!Attributes of the RTPSParticipant
	RTPSParticipantAttributes m_att;
	//!Guid of the RTPSParticipant.
	const GUID_t m_guid;
	//! Sending resources. - DEPRECATED -Stays commented for reference purposes
	// ResourceSend* mp_send_thr;
	//! Event Resource
	ResourceEvent* mp_event_thr;
	//! BuiltinProtocols of this RTPSParticipant
	BuiltinProtocols* mp_builtinProtocols;
	//!Semaphore to wait for the listen thread creation.
	boost::interprocess::interprocess_semaphore* mp_ResourceSemaphore;
	//!Id counter to correctly assign the ids to writers and readers.
	uint32_t IdCounter;
	//!Writer List.
	std::vector<RTPSWriter*> m_allWriterList;
	//!Reader List
	std::vector<RTPSReader*> m_allReaderList;
	//!Listen thread list.
	//!Writer List.
	std::vector<RTPSWriter*> m_userWriterList;
	//!Reader List
	std::vector<RTPSReader*> m_userReaderList;

	//!Network Factory
	NetworkFactory m_network_Factory;
	//!ReceiverControlBlock list - encapsulates all associated resources on a Receiving element
	std::list<ReceiverControlBlock> m_receiverResourcelist;
	//!SenderResource List
	std::vector<SenderResource> m_senderResource;

	//!Listen Resource list - DEPRECATED - Stays commented for reference purposes
	// std::vector<ListenResource*> m_listenResourceList;
	//!Participant Listener
	RTPSParticipantListener* mp_participantListener;
	//!Pointer to the user participant
	RTPSParticipant* mp_userParticipant;

    RTPSParticipantImpl& operator=(const RTPSParticipantImpl&) NON_COPYABLE_CXX11;

	/**
	 * Method to check if a specific entityId already exists in this RTPSParticipant
	 * @param ent EnityId to check
	 * @param kind Endpoint Kind.
	 * @return True if exists.
	 */
	bool existsEntityId(const EntityId_t& ent,EndpointKind_t kind) const;

	/**
	 * Assign an endpoint to the ReceiverResources, based on its LocatorLists.
	 * @param endp Pointer to the endpoint.
	 * @param isBuiltin Boolean indicating if it is builtin.
	 * @return True if correct.
	 */
	bool assignEndpointListenResources(Endpoint* endp,bool isBuiltin);

	/** Assign an endpoint to the ReceiverResources as specified specifically on parameter list
	 * @param pend Pointer to the endpoint.
	 * @param lit Locator list iterator.
	 * @param isMulticast Boolean indicating that is multicast.
	 * @param isFixed Boolean indicating that is a fixed listenresource.
	 * @return True if assigned.
	 */
	bool assignEndpoint2LocatorList(Endpoint* pend,LocatorList_t& list,bool isMulticast,bool isFixed);

	/** Create the new ReceiverResources needed for a new Locator, contains the calls to assignEndpointListenResources
		and consequently assignEndpoint2LocatorList
		@param pend - Pointer to the endpoint which triggered the creation of the Receivers
	*/
	bool createAndAssociateReceiverswithEndpoint(Endpoint * pend, bool isBuiltIn);
	
	/** Function to be called from a new thread, which takes cares of performing a blocking receive 
		operation on the ReceiveResource
		@param buffer - Position of the buffer we use to store data
		@param locator - Locator that triggered the creation of the resource
	*/
	void performListenOperation(ReceiverControlBlock *receiver, Locator_t input_locator);

	/** Create non-existent SendResources based on the Locator list of the entity
		@param pend - Pointer to the endpoint whose SenderResources are to be created
	*/
	bool createSendResources(Endpoint *pend);
	
	/** When we want to create a new Resource but the physical channel specified by the Locator
	    can not be opened, we want to mutate the Locator to open a more or less equivalent channel.
		@param loc -  Locator we want to change
	*/
	Locator_t applyLocatorAdaptRule(Locator_t loc);
	
	//!Participant Mutex
	boost::recursive_mutex* mp_mutex;
	//!ListenThreadId
	uint32_t m_threadID;
public:
	/**
	 * Create a Writer in this RTPSParticipant.
	 * @param Writer Pointer to pointer of the Writer, used as output. Only valid if return==true.
	 * @param param WriterAttributes to define the Writer.
	 * @param entityId EntityId assigned to the Writer.
	 * @param isBuiltin Bool value indicating if the Writer is builtin (Discovery or Liveliness protocol) or is created for the end user.
	 * @return True if the Writer was correctly created.
	 */
	bool createWriter(RTPSWriter** Writer, WriterAttributes& param,WriterHistory* hist,WriterListener* listen,
			const EntityId_t& entityId = c_EntityId_Unknown,bool isBuiltin = false);

	/**
	 * Create a Reader in this RTPSParticipant.
	 * @param Reader Pointer to pointer of the Reader, used as output. Only valid if return==true.
	 * @param param ReaderAttributes to define the Reader.
	 * @param entityId EntityId assigned to the Reader.
	 * @param isBuiltin Bool value indicating if the Reader is builtin (Discovery or Liveliness protocol) or is created for the end user.
	 * @return True if the Reader was correctly created.
	 */
	bool createReader(RTPSReader** Reader, ReaderAttributes& param,ReaderHistory* hist,ReaderListener* listen,
				const EntityId_t& entityId = c_EntityId_Unknown,bool isBuiltin = false, bool enable = true);

    bool enableReader(RTPSReader *reader, bool isBuiltin = false);

	/**
	* Register a Writer in the BuiltinProtocols.
	* @param Writer Pointer to the RTPSWriter.
	* @param topicAtt TopicAttributes of the Writer.
	* @param wqos WriterQos.
	* @return True if correctly registered.
	*/
	bool registerWriter(RTPSWriter* Writer,TopicAttributes& topicAtt,WriterQos& wqos);

	/**
	* Register a Reader in the BuiltinProtocols.
	* @param Reader Pointer to the RTPSReader.
	* @param topicAtt TopicAttributes of the Reader.
	* @param rqos ReaderQos.
	* @return  True if correctly registered.
	*/
	bool registerReader(RTPSReader* Reader,TopicAttributes& topicAtt,ReaderQos& rqos);

	/**
	* Update local writer QoS
	* @param Writer Writer to update
	* @param wqos New QoS for the writer
	* @return True on success
	*/
	bool updateLocalWriter(RTPSWriter* Writer,WriterQos& wqos);

	/**
	* Update local reader QoS
	* @param Reader Reader to update
	* @param rqos New QoS for the reader
	* @return True on success
	*/
	bool updateLocalReader(RTPSReader* Reader, ReaderQos& rqos);



	/**
	* Get the participant attributes
	* @return Participant attributes
	*/
	inline RTPSParticipantAttributes& getAttributes() {return m_att;};

	/**
	* Delete a user endpoint
	* @param Endpoint to delete
	* @return True on success
	*/
	bool deleteUserEndpoint(Endpoint*);

	/**
	* Get the begin of the user reader list
	* @return Iterator pointing to the begin of the user reader list
	*/
	std::vector<RTPSReader*>::iterator userReadersListBegin(){return m_userReaderList.begin();};

	/**
	* Get the end of the user reader list
	* @return Iterator pointing to the end of the user reader list
	*/
	std::vector<RTPSReader*>::iterator userReadersListEnd(){return m_userReaderList.end();};

	/**
	* Get the begin of the user writer list
	* @return Iterator pointing to the begin of the user writer list
	*/
	std::vector<RTPSWriter*>::iterator userWritersListBegin(){return m_userWriterList.begin();};

	/**
	* Get the end of the user writer list
	* @return Iterator pointing to the end of the user writer list
	*/
	std::vector<RTPSWriter*>::iterator userWritersListEnd(){return m_userWriterList.end();};

	/** Helper function that creates ReceiverResources based on a Locator_t List, possibly mutating
	some and updating the list. DOES NOT associate endpoints with it.
	@param Locator_list - Locator list to be used to create the ReceiverResources
	@param ApplyMutation - True if we want to create a Resource with a "similar" locator if the one we provide is unavailable
	*/
	bool createReceiverResources(LocatorList_t& Locator_list, bool ApplyMutation);

};

}
} /* namespace rtps */
} /* namespace eprosima */
#endif
#endif /* RTPSParticipant_H_ */





