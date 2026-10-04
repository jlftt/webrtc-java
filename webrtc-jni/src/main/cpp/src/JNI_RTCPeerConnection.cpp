/*
 * Copyright 2019 Alex Andres
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "JNI_RTCPeerConnection.h"
#include "api/CreateSessionDescriptionObserver.h"
#include "api/SetSessionDescriptionObserver.h"
#include "api/RTCAnswerOptions.h"
#include "api/RTCConfiguration.h"
#include "api/RTCDataChannelInit.h"
#include "api/RTCIceCandidate.h"
#include "api/RTCOfferOptions.h"
#include "api/RTCSessionDescription.h"
#include "api/RTCStatsCollectorCallback.h"
#include "api/WebRTCUtils.h"
#include "JavaArray.h"
#include "JavaEnums.h"
#include "JavaFactories.h"
#include "JavaNullPointerException.h"
#include "JavaRef.h"
#include "JavaRuntimeException.h"
#include "JavaString.h"
#include "JavaUtils.h"

#include "api/peer_connection_interface.h"

#include <string>

// Takes this call's own reference to the PeerConnection, under the Java object's monitor.
// close() swaps the handle out under the same monitor before it drops Java's reference, so a
// call racing with close() on another thread either sees no handle or keeps the
// PeerConnection alive until it returns, and can never use one that was already freed.
// The monitor is only held to read the handle, never across a call into WebRTC.
static webrtc::scoped_refptr<webrtc::PeerConnectionInterface> AcquirePeerConnection(JNIEnv * env, jobject caller)
{
	env->MonitorEnter(caller);
	webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pc(GetHandle<webrtc::PeerConnectionInterface>(env, caller));
	env->MonitorExit(caller);

	return pc;
}

JNIEXPORT jobjectArray JNICALL Java_dev_kastle_webrtc_RTCPeerConnection_getSenders
(JNIEnv * env, jobject caller)
{
	webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pcRef = AcquirePeerConnection(env, caller);
	webrtc::PeerConnectionInterface * pc = pcRef.get();
	CHECK_HANDLEV(pc, nullptr);

	jni::JavaLocalRef<jobjectArray> objectArray;

	try {
		objectArray = jni::createObjectArray(env, pc->GetSenders());
	}
	catch (...) {
		ThrowCxxJavaException(env);
	}

	return objectArray.release();
}

JNIEXPORT jobjectArray JNICALL Java_dev_kastle_webrtc_RTCPeerConnection_getReceivers
(JNIEnv * env, jobject caller)
{
	webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pcRef = AcquirePeerConnection(env, caller);
	webrtc::PeerConnectionInterface * pc = pcRef.get();
	CHECK_HANDLEV(pc, nullptr);

	jni::JavaLocalRef<jobjectArray> objectArray;

	try {
		objectArray = jni::createObjectArray(env, pc->GetReceivers());
	}
	catch (...) {
		ThrowCxxJavaException(env);
	}

	return objectArray.release();
}

JNIEXPORT jobjectArray JNICALL Java_dev_kastle_webrtc_RTCPeerConnection_getTransceivers
(JNIEnv * env, jobject caller)
{
	webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pcRef = AcquirePeerConnection(env, caller);
	webrtc::PeerConnectionInterface * pc = pcRef.get();
	CHECK_HANDLEV(pc, nullptr);

	jni::JavaLocalRef<jobjectArray> objectArray;

	try {
		objectArray = jni::createObjectArray(env, pc->GetTransceivers());
	}
	catch (...) {
		ThrowCxxJavaException(env);
	}

	return objectArray.release();
}

JNIEXPORT jobject JNICALL Java_dev_kastle_webrtc_RTCPeerConnection_createDataChannel
(JNIEnv * env, jobject caller, jstring jLabel, jobject jDict)
{
	if (jLabel == nullptr) {
		env->Throw(jni::JavaNullPointerException(env, "Label must not be null"));
		return nullptr;
	}
	if (jDict == nullptr) {
		env->Throw(jni::JavaNullPointerException(env, "RTCDataChannelInit must not be null"));
		return nullptr;
	}

	webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pcRef = AcquirePeerConnection(env, caller);
	webrtc::PeerConnectionInterface * pc = pcRef.get();
	CHECK_HANDLEV(pc, nullptr);

	std::string label = jni::JavaString::toNative(env, jni::JavaLocalRef<jstring>(env, jLabel));
	webrtc::DataChannelInit dict = jni::RTCDataChannelInit::toNative(env, jni::JavaLocalRef<jobject>(env, jDict));

	try {
		auto result = pc->CreateDataChannelOrError(label, &dict);

		if (!result.ok()) {
			env->Throw(jni::JavaRuntimeException(env, "Create DataChannel failed: %s %s",
				ToString(result.error().type()), result.error().message()));

			return nullptr;
		}

		auto dataChannel = result.MoveValue();

		return jni::JavaFactories::create(env, dataChannel.release()).release();
	}
	catch (...) {
		ThrowCxxJavaException(env);
		return nullptr;
	}
}

JNIEXPORT void JNICALL Java_dev_kastle_webrtc_RTCPeerConnection_createOffer
(JNIEnv * env, jobject caller, jobject jOptions, jobject jObserver)
{
	if (jOptions == nullptr) {
		env->Throw(jni::JavaNullPointerException(env, "RTCOfferOptions must not be null"));
		return;
	}
	if (jObserver == nullptr) {
		env->Throw(jni::JavaNullPointerException(env, "CreateSessionDescriptionObserver must not be null"));
		return;
	}

	webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pcRef = AcquirePeerConnection(env, caller);
	webrtc::PeerConnectionInterface * pc = pcRef.get();
	CHECK_HANDLE(pc);

	try {
		auto options = jni::RTCOfferOptions::toNative(env, jni::JavaLocalRef<jobject>(env, jOptions));
		auto observer = new webrtc::RefCountedObject<jni::CreateSessionDescriptionObserver>(env, jni::JavaGlobalRef<jobject>(env, jObserver));

		pc->CreateOffer(observer, options);
	}
	catch (...) {
		ThrowCxxJavaException(env);
	}
}

JNIEXPORT void JNICALL Java_dev_kastle_webrtc_RTCPeerConnection_createAnswer
(JNIEnv * env, jobject caller, jobject jOptions, jobject jObserver)
{
	if (jOptions == nullptr) {
		env->Throw(jni::JavaNullPointerException(env, "RTCAnswerOptions must not be null"));
		return;
	}
	if (jObserver == nullptr) {
		env->Throw(jni::JavaNullPointerException(env, "CreateSessionDescriptionObserver must not be null"));
		return;
	}

	webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pcRef = AcquirePeerConnection(env, caller);
	webrtc::PeerConnectionInterface * pc = pcRef.get();
	CHECK_HANDLE(pc);

	try {
		auto options = jni::RTCAnswerOptions::toNative(env, jni::JavaLocalRef<jobject>(env, jOptions));
		auto observer = new webrtc::RefCountedObject<jni::CreateSessionDescriptionObserver>(env, jni::JavaGlobalRef<jobject>(env, jObserver));

		pc->CreateAnswer(observer, options);
	}
	catch (...) {
		ThrowCxxJavaException(env);
	}
}

JNIEXPORT jobject JNICALL Java_dev_kastle_webrtc_RTCPeerConnection_getCurrentLocalDescription
(JNIEnv * env, jobject caller)
{
	webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pcRef = AcquirePeerConnection(env, caller);
	webrtc::PeerConnectionInterface * pc = pcRef.get();
	CHECK_HANDLEV(pc, nullptr);

	if (!pc->current_local_description()) {
		return nullptr;
	}

	return jni::RTCSessionDescription::toJava(env, pc->current_local_description()).release();
}

JNIEXPORT jobject JNICALL Java_dev_kastle_webrtc_RTCPeerConnection_getLocalDescription
(JNIEnv * env, jobject caller)
{
	webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pcRef = AcquirePeerConnection(env, caller);
	webrtc::PeerConnectionInterface * pc = pcRef.get();
	CHECK_HANDLEV(pc, nullptr);

	if (!pc->local_description()) {
		return nullptr;
	}

	return jni::RTCSessionDescription::toJava(env, pc->local_description()).release();
}

JNIEXPORT jobject JNICALL Java_dev_kastle_webrtc_RTCPeerConnection_getPendingLocalDescription
(JNIEnv * env, jobject caller)
{
	webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pcRef = AcquirePeerConnection(env, caller);
	webrtc::PeerConnectionInterface * pc = pcRef.get();
	CHECK_HANDLEV(pc, nullptr);

	if (!pc->pending_local_description()) {
		return nullptr;
	}

	return jni::RTCSessionDescription::toJava(env, pc->pending_local_description()).release();
}

JNIEXPORT jobject JNICALL Java_dev_kastle_webrtc_RTCPeerConnection_getCurrentRemoteDescription
(JNIEnv * env, jobject caller)
{
	webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pcRef = AcquirePeerConnection(env, caller);
	webrtc::PeerConnectionInterface * pc = pcRef.get();
	CHECK_HANDLEV(pc, nullptr);

	if (!pc->current_remote_description()) {
		return nullptr;
	}

	return jni::RTCSessionDescription::toJava(env, pc->current_remote_description()).release();
}

JNIEXPORT jobject JNICALL Java_dev_kastle_webrtc_RTCPeerConnection_getRemoteDescription
(JNIEnv * env, jobject caller)
{
	webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pcRef = AcquirePeerConnection(env, caller);
	webrtc::PeerConnectionInterface * pc = pcRef.get();
	CHECK_HANDLEV(pc, nullptr);

	if (!pc->remote_description()) {
		return nullptr;
	}

	return jni::RTCSessionDescription::toJava(env, pc->remote_description()).release();
}

JNIEXPORT jobject JNICALL Java_dev_kastle_webrtc_RTCPeerConnection_getPendingRemoteDescription
(JNIEnv * env, jobject caller)
{
	webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pcRef = AcquirePeerConnection(env, caller);
	webrtc::PeerConnectionInterface * pc = pcRef.get();
	CHECK_HANDLEV(pc, nullptr);

	if (!pc->pending_remote_description()) {
		return nullptr;
	}

	return jni::RTCSessionDescription::toJava(env, pc->pending_remote_description()).release();
}

JNIEXPORT void JNICALL Java_dev_kastle_webrtc_RTCPeerConnection_setLocalDescription
(JNIEnv * env, jobject caller, jobject jSessionDesc, jobject jobserver)
{
	if (jSessionDesc == nullptr) {
		env->Throw(jni::JavaNullPointerException(env, "RTCSessionDescription must not be null"));
		return;
	}
	if (jobserver == nullptr) {
		env->Throw(jni::JavaNullPointerException(env, "SetSessionDescriptionObserver must not be null"));
		return;
	}

	webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pcRef = AcquirePeerConnection(env, caller);
	webrtc::PeerConnectionInterface * pc = pcRef.get();
	CHECK_HANDLE(pc);

	try {
		auto desc = jni::RTCSessionDescription::toNative(env, jni::JavaLocalRef<jobject>(env, jSessionDesc));
		auto observer = new webrtc::RefCountedObject<jni::SetSessionDescriptionObserver>(env, jni::JavaGlobalRef<jobject>(env, jobserver));

		pc->SetLocalDescription(observer, desc.release());
	}
	catch (...) {
		ThrowCxxJavaException(env);
	}
}

JNIEXPORT void JNICALL Java_dev_kastle_webrtc_RTCPeerConnection_setRemoteDescription
(JNIEnv * env, jobject caller, jobject jSessionDesc, jobject jobserver)
{
	if (jSessionDesc == nullptr) {
		env->Throw(jni::JavaNullPointerException(env, "RTCSessionDescription must not be null"));
		return;
	}
	if (jobserver == nullptr) {
		env->Throw(jni::JavaNullPointerException(env, "SetSessionDescriptionObserver must not be null"));
		return;
	}

	webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pcRef = AcquirePeerConnection(env, caller);
	webrtc::PeerConnectionInterface * pc = pcRef.get();
	CHECK_HANDLE(pc);

	try {
		auto desc = jni::RTCSessionDescription::toNative(env, jni::JavaLocalRef<jobject>(env, jSessionDesc));
		auto observer = new webrtc::RefCountedObject<jni::SetSessionDescriptionObserver>(env, jni::JavaGlobalRef<jobject>(env, jobserver));

		pc->SetRemoteDescription(observer, desc.release());
	}
	catch (...) {
		ThrowCxxJavaException(env);
	}
}

JNIEXPORT void JNICALL Java_dev_kastle_webrtc_RTCPeerConnection_addIceCandidate
(JNIEnv * env, jobject caller, jobject jCandidate)
{
	if (jCandidate == nullptr) {
		env->Throw(jni::JavaNullPointerException(env, "RTCIceCandidate must not be null"));
		return;
	}

	webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pcRef = AcquirePeerConnection(env, caller);
	webrtc::PeerConnectionInterface * pc = pcRef.get();
	CHECK_HANDLE(pc);

	try {
		auto candidate = jni::RTCIceCandidate::toNative(env, jni::JavaLocalRef<jobject>(env, jCandidate));

		pc->AddIceCandidate(candidate.get());
	}
	catch (...) {
		ThrowCxxJavaException(env);
	}
}

JNIEXPORT void JNICALL Java_dev_kastle_webrtc_RTCPeerConnection_removeIceCandidates
(JNIEnv * env, jobject caller, jobject jCandidates)
{
	if (jCandidates == nullptr) {
		return;
	}

	webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pcRef = AcquirePeerConnection(env, caller);
	webrtc::PeerConnectionInterface * pc = pcRef.get();
	CHECK_HANDLE(pc);

	try {
		auto candidates = jni::JavaArray::toNativeVector<webrtc::Candidate>(env,
			jni::static_java_ref_cast<jobjectArray>(env, jni::JavaLocalRef<jobject>(env, jCandidates)),
			&jni::RTCIceCandidate::toNativeCricket);

		if (!pc->RemoveIceCandidates(candidates)) {
			env->Throw(jni::JavaRuntimeException(env, "Remove ICE candidates from the peer connection failed"));
		}
	}
	catch (...) {
		ThrowCxxJavaException(env);
	}
}

JNIEXPORT jobject JNICALL Java_dev_kastle_webrtc_RTCPeerConnection_getSignalingState
(JNIEnv * env, jobject caller)
{
	webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pcRef = AcquirePeerConnection(env, caller);
	webrtc::PeerConnectionInterface * pc = pcRef.get();
	CHECK_HANDLE_DEFAULT(pc, jni::JavaEnums::toJava(env, webrtc::PeerConnectionInterface::SignalingState::kClosed).release());

	return jni::JavaEnums::toJava(env, pc->signaling_state()).release();
}

JNIEXPORT jobject JNICALL Java_dev_kastle_webrtc_RTCPeerConnection_getIceGatheringState
(JNIEnv * env, jobject caller)
{
	webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pcRef = AcquirePeerConnection(env, caller);
	webrtc::PeerConnectionInterface * pc = pcRef.get();
	CHECK_HANDLE_DEFAULT(pc, jni::JavaEnums::toJava(env, webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringNew).release());

	return jni::JavaEnums::toJava(env, pc->ice_gathering_state()).release();
}

JNIEXPORT jobject JNICALL Java_dev_kastle_webrtc_RTCPeerConnection_getIceConnectionState
(JNIEnv * env, jobject caller)
{
	webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pcRef = AcquirePeerConnection(env, caller);
	webrtc::PeerConnectionInterface * pc = pcRef.get();
	CHECK_HANDLE_DEFAULT(pc, jni::JavaEnums::toJava(env, webrtc::PeerConnectionInterface::IceConnectionState::kIceConnectionClosed).release());

	return jni::JavaEnums::toJava(env, pc->ice_connection_state()).release();
}

JNIEXPORT jobject JNICALL Java_dev_kastle_webrtc_RTCPeerConnection_getConnectionState
(JNIEnv * env, jobject caller)
{
	webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pcRef = AcquirePeerConnection(env, caller);
	webrtc::PeerConnectionInterface * pc = pcRef.get();
	CHECK_HANDLE_DEFAULT(pc, jni::JavaEnums::toJava(env, webrtc::PeerConnectionInterface::PeerConnectionState::kClosed).release());

	return jni::JavaEnums::toJava(env, pc->peer_connection_state()).release();
}

JNIEXPORT jobject JNICALL Java_dev_kastle_webrtc_RTCPeerConnection_getConfiguration
(JNIEnv * env, jobject caller)
{
	webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pcRef = AcquirePeerConnection(env, caller);
	webrtc::PeerConnectionInterface * pc = pcRef.get();
	CHECK_HANDLEV(pc, nullptr);

	return jni::RTCConfiguration::toJava(env, pc->GetConfiguration()).release();
}

JNIEXPORT void JNICALL Java_dev_kastle_webrtc_RTCPeerConnection_setConfiguration
(JNIEnv * env, jobject caller, jobject jConfig)
{
	if (jConfig == nullptr) {
		env->Throw(jni::JavaNullPointerException(env, "RTCConfiguration must not be null"));
		return;
	}

	webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pcRef = AcquirePeerConnection(env, caller);
	webrtc::PeerConnectionInterface * pc = pcRef.get();
	CHECK_HANDLE(pc);

	auto config = jni::RTCConfiguration::toNative(env, jni::JavaLocalRef<jobject>(env, jConfig));

	webrtc::RTCError error = pc->SetConfiguration(config);

	if (!error.ok()) {
		env->Throw(jni::JavaRuntimeException(env, jni::RTCErrorToString(error).c_str()));
	}
}

JNIEXPORT void JNICALL Java_dev_kastle_webrtc_RTCPeerConnection_getStats__Ldev_kastle_webrtc_RTCStatsCollectorCallback_2
(JNIEnv * env, jobject caller, jobject jcallback)
{
	webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pcRef = AcquirePeerConnection(env, caller);
	webrtc::PeerConnectionInterface * pc = pcRef.get();
	CHECK_HANDLE(pc);

	if (jcallback == nullptr) {
		env->Throw(jni::JavaNullPointerException(env, "RTCStatsCollectorCallback is null"));
		return;
	}

	auto callback = new webrtc::RefCountedObject<jni::RTCStatsCollectorCallback>(env, jni::JavaGlobalRef<jobject>(env, jcallback));

	pc->GetStats(callback);
}

JNIEXPORT void JNICALL Java_dev_kastle_webrtc_RTCPeerConnection_restartIce
(JNIEnv * env, jobject caller)
{
	webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pcRef = AcquirePeerConnection(env, caller);
	webrtc::PeerConnectionInterface * pc = pcRef.get();
	CHECK_HANDLE(pc);

	pc->RestartIce();
}

JNIEXPORT void JNICALL Java_dev_kastle_webrtc_RTCPeerConnection_close
(JNIEnv * env, jobject caller)
{
	// Taken out of the Java object first, under the monitor AcquirePeerConnection() reads it
	// under: no other call can pick the handle up once this one is about to free it, and a
	// second close() finds nothing to free.
	env->MonitorEnter(caller);
	webrtc::PeerConnectionInterface * pc = GetHandle<webrtc::PeerConnectionInterface>(env, caller);
	if (pc != nullptr) {
		SetHandle<std::nullptr_t>(env, caller, nullptr);
	}
	env->MonitorExit(caller);
	CHECK_HANDLE(pc);

	try {
		pc->Close();

		ClearNativeObserver<webrtc::PeerConnectionObserver>(env, caller, "observerHandle");

		// Drop the owning reference taken when the PeerConnection was handed
		// to Java in PeerConnectionFactory::createPeerConnection.
		pc->Release();
	}
	catch (...) {
		ThrowCxxJavaException(env);
	}
}