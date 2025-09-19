import fastdds
from .qos_profile import PublisherQosProfile, SubscriberQosProfile
from typing import Callable, Dict


class WriterListener (fastdds.DataWriterListener) :
    def __init__(self, writer) :
        super().__init__()
        self._writer = writer

    def on_publication_matched(self, writer, info) :
        if (0 < info.current_count_change) :
            print ("Publisher matched subscriber {}".format(info.last_subscription_handle))
            self._writer.is_matched = True
        else :
            print ("Publisher unmatched subscriber {}".format(info.last_subscription_handle))
            self._writer.is_matched = False

class ReaderListener(fastdds.DataReaderListener):
    def __init__(self, reader, callback: Callable, topic_data_type):
        super().__init__()
        self._reader = reader
        self.callback = callback 
        self.topic_data_type = topic_data_type

    def on_data_available(self, reader):
        info = fastdds.SampleInfo()
        data = self.topic_data_type
        if reader.take_next_sample(data, info) == fastdds.RETCODE_OK:
            if info.valid_data:
                self.callback(data)
            else:
                print("Warning: Received invalid data sample")
        else:
            print("Error: Failed to take sample from reader")

    def on_subscription_matched(self, reader, info) :
        if (0 < info.current_count_change) :
            print ("Subscriber matched publisher {}".format(info.last_publication_handle))
            self._reader.is_matched = True
        else :
            print ("Subscriber unmatched publisher {}".format(info.last_publication_handle))
            self._reader.is_matched = False
        if (info.current_count > 1) :
            print("Warning: Subscriber matched multiple publishers")

class Publisher:
    def __init__(self, publisher, topic, writer_qos):
        self.is_matched = False
        writer_listener = WriterListener(self)
        self._writer = publisher.create_datawriter(topic, writer_qos, writer_listener)

    def publish(self, data):
        self._writer.write(data)

class Subscriber:
    def __init__(self, subscriber, topic, reader_qos, callback, topic_data_type):
        self.is_matched = False
        reader_listener = ReaderListener(self, callback, topic_data_type)
        self._reader = subscriber.create_datareader(topic, reader_qos, reader_listener)

class DDSInterface:
    # TODO: customize participant_qos
    def __init__(self, domain_id: int =0 , participant_qos =0):
        self._factory = fastdds.DomainParticipantFactory.get_instance()
        participant_qos = fastdds.DomainParticipantQos()
        participant_qos.transport().use_builtin_transports = True
        self._participant = self._factory.create_participant(domain_id, participant_qos)
        self._topics: Dict[str, fastdds.Topic] = {}

    def __del__(self):
        self._close()

    def _close(self):
        self._participant.delete_contained_entities()
        self._factory.delete_participant(self._participant)

    def _register_type(self, topic_name: str, topic_data_type):
        type_support = fastdds.TypeSupport(topic_data_type)
        if self._participant.register_type(type_support) != fastdds.RETCODE_OK:
            print(f"Error: Failed to register type for topic {topic_name}")

    def _create_topic(self, topic_name: str, topic_data_type, topic_qos):
        if topic_name not in self._topics:
            topic_data_type.set_name(topic_name)
            self._register_type(topic_name, topic_data_type)
            topic = self._participant.create_topic(topic_name, topic_data_type.get_name(), topic_qos)
            self._topics[topic_name] = topic
            return self._topics[topic_name]
        else:
            raise ValueError(f"Topic {topic_name} already exists")
        
# ------------------------------User interface-------------------------------------------------------------
    
    def create_publisher(self, topic_name: str, topic_data_pub_sub_type, publisher_qos: PublisherQosProfile = PublisherQosProfile.DEFAULT):
        if not isinstance(topic_name, str):
            raise TypeError("topic_name must be a string")
        if not isinstance(publisher_qos, PublisherQosProfile):
            raise TypeError("publisher_qos must be an instance of PublisherQosProfile")
        
        topic = self._create_topic(topic_name, topic_data_pub_sub_type, publisher_qos.topic_qos)
        publisher = self._participant.create_publisher(publisher_qos.publisher_qos)
        writer = Publisher(publisher, topic, publisher_qos.writer_qos)
        return writer

    def create_subscriber(self, topic_name: str, topic_data_pub_sub_type, topic_data_type, callback: Callable, subscriber_qos: SubscriberQosProfile = SubscriberQosProfile.DEFAULT):
        if not isinstance(topic_name, str):
            raise TypeError("topic_name must be a string")
        if not callable(callback):
            raise TypeError("callback must be a callable function")
        if not isinstance(subscriber_qos, SubscriberQosProfile):
            raise TypeError("subscriber_qos must be an instance of SubscriberQosProfile")
        
        topic = self._create_topic(topic_name, topic_data_pub_sub_type, subscriber_qos.topic_qos)
        subscriber = self._participant.create_subscriber(subscriber_qos.subscriber_qos)
        reader = Subscriber(subscriber, topic, subscriber_qos.reader_qos, callback, topic_data_type)
        return reader
    
    # TODO: add service/action related interfaces

        
        
