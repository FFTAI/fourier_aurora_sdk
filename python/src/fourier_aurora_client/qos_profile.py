from dataclasses import dataclass, field
from typing import ClassVar
from fastdds import TopicQos, SubscriberQos, PublisherQos, DataReaderQos, DataWriterQos
import fastdds

@dataclass(frozen=True)
class SubscriberQosProfile:
    topic_qos: TopicQos           = field(default_factory=TopicQos)
    subscriber_qos: SubscriberQos = field(default_factory=SubscriberQos)
    reader_qos: DataReaderQos     = field(default_factory=DataReaderQos)

    @classmethod
    def reliable(cls) -> "SubscriberQosProfile":
        profile = cls()
        profile.topic_qos.durability   = fastdds.TRANSIENT_LOCAL
        profile.reader_qos.reliability = fastdds.RELIABLE
        return profile

    @classmethod
    def best_effort(cls) -> "SubscriberQosProfile":
        profile = cls()
        profile.topic_qos.durability   = fastdds.VOLATILE
        profile.reader_qos.reliability = fastdds.BEST_EFFORT
        return profile
    
    DEFAULT:     ClassVar["SubscriberQosProfile"]
    RELIABLE:    ClassVar["SubscriberQosProfile"]
    BEST_EFFORT: ClassVar["SubscriberQosProfile"]

@dataclass(frozen=True)
class PublisherQosProfile:
    topic_qos: TopicQos           = field(default_factory=TopicQos)
    publisher_qos: PublisherQos   = field(default_factory=PublisherQos)
    writer_qos: DataWriterQos     = field(default_factory=DataWriterQos)

    @classmethod
    def reliable(cls) -> "PublisherQosProfile":
        profile = cls()
        profile.topic_qos.durability   = fastdds.TRANSIENT_LOCAL
        profile.writer_qos.reliability = fastdds.RELIABLE
        return profile

    @classmethod
    def best_effort(cls) -> "PublisherQosProfile":
        profile = cls()
        profile.topic_qos.durability   = fastdds.VOLATILE
        profile.writer_qos.reliability = fastdds.BEST_EFFORT
        return profile
    
    DEFAULT:     ClassVar["PublisherQosProfile"]
    RELIABLE:    ClassVar["PublisherQosProfile"]
    BEST_EFFORT: ClassVar["PublisherQosProfile"]

SubscriberQosProfile.DEFAULT     = SubscriberQosProfile()
SubscriberQosProfile.RELIABLE    = SubscriberQosProfile.reliable()
SubscriberQosProfile.BEST_EFFORT = SubscriberQosProfile.best_effort()
PublisherQosProfile.DEFAULT     = PublisherQosProfile()
PublisherQosProfile.RELIABLE    = PublisherQosProfile.reliable()
PublisherQosProfile.BEST_EFFORT = PublisherQosProfile.best_effort()




























