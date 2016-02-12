/*!
 * \file heapstatsMBean.cpp
 * \brief JNI implementation for HeapStatsMBean.
 * Copyright (C) 2014-2015 Yasumasa Suenaga
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <jni.h>

#include <limits.h>

#include <list>

#include "globals.hpp"
#include "configuration.hpp"
#include "heapstatsMBean.hpp"

/* Variables */
static jclass mapCls = NULL;
static jmethodID map_ctor = NULL;
static jmethodID map_put = NULL;

static jclass boolCls = NULL;
static jmethodID boolValue = NULL;
static jobject boolFalse = NULL;
static jobject boolTrue = NULL;

static jobjectArray logLevelArray;
static jobjectArray rankOrderArray;

static jclass integerCls = NULL;
static jmethodID intValue = NULL;
static jmethodID intValueOf = NULL;

static jclass longCls = NULL;
static jmethodID longValue = NULL;
static jmethodID longValueOf = NULL;

/*!
 * \brief Raise Java Exception.
 *
 * \param env     Pointer of JNI environment.
 * \param cls_sig Class signature of Throwable.
 * \param message Error message.
 */
static void raiseException(JNIEnv *env, const char *cls_sig,
                           const char *message) {
  jthrowable ex = env->ExceptionOccurred();
  if (ex != NULL) {
    env->Throw(ex);
    return;
  }

  jclass ex_cls = env->FindClass(cls_sig);
  env->ThrowNew(ex_cls, message);
}

/*!
 * \brief Load jclass object.
 *        This function returns jclass as JNI global object.
 *
 * \param env       Pointer of JNI environment.
 * \param className Class name to load.
 * \return jclass object.
 */
static jclass loadClassGlobal(JNIEnv *env, const char *className) {
  jclass cls = env->FindClass(className);

  if (cls != NULL) {
    cls = (jclass)env->NewGlobalRef(cls);

    if (cls == NULL) {
      raiseException(env, "java/lang/RuntimeException",
                     "Could not get JNI Global value.");
    }

  } else {
    raiseException(env, "java/lang/NoClassDefFoundError", className);
  }

  return cls;
}

/*!
 * \brief Preparation for Map object.
 *        This function initialize JNI variables for LinkedHashMap to use in
 *        getConfigurationList0()
 *
 * \param env Pointer of JNI environment.
 * \return true if succeeded.
 */
static bool prepareForMapObject(JNIEnv *env) {
  mapCls = loadClassGlobal(env, "java/util/LinkedHashMap");
  if (mapCls == NULL) {
    return false;
  }

  map_ctor = env->GetMethodID(mapCls, "<init>", "()V");
  map_put = env->GetMethodID(
      mapCls, "put",
      "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
  if ((map_ctor == NULL) || (map_put == NULL)) {
    raiseException(env, "java/lang/RuntimeException",
                   "Could not get methods in LinkedHashMap.");
    return false;
  }

  return true;
}

/*!
 * \brief Preparation for Boolean object.
 *        This function initialize JNI variables for Boolean.TRUE and FALSE.
 *
 * \param env       Pointer of JNI environment.
 * \param fieldName Field name in Boolean. It must be "TRUE" or "FALSE".
 * \param target    jobject to store value of field.
 * \return true if succeeded.
 */
static bool prepareForBoolean(JNIEnv *env, const char *fieldName,
                              jobject *target) {
  if (boolCls == NULL) {
    boolCls = loadClassGlobal(env, "java/lang/Boolean");

    if (boolCls == NULL) {
      return false;
    }

    boolValue = env->GetMethodID(boolCls, "booleanValue", "()Z");

    if (boolValue == NULL) {
      raiseException(env, "java/lang/RuntimeException",
                     "Could not find Boolean method.");
      return false;
    }
  }

  jfieldID field =
      env->GetStaticFieldID(boolCls, fieldName, "Ljava/lang/Boolean;");
  if (field == NULL) {
    raiseException(env, "java/lang/RuntimeException",
                   "Could not find Boolean field.");
    return false;
  }

  *target = env->GetStaticObjectField(boolCls, field);
  if (*target != NULL) {
    *target = env->NewGlobalRef(*target);

    if (*target == NULL) {
      raiseException(env, "java/lang/RuntimeException",
                     "Could not get JNI Global value.");
      return false;
    }

  } else {
    raiseException(env, "java/lang/RuntimeException",
                   "Could not get Boolean value.");
    return false;
  }

  return true;
}

/*!
 * \brief Preparation for Enum object in HeapStatsMBean.
 *
 * \param env       Pointer of JNI environment.
 * \param className Class name to initialize.
 *                  It must be "LogLevel" or "RankOrder".
 * \param target    jobjectArray to store values in enum.
 * \return true if succeeded.
 */
static bool prepareForEnumObject(JNIEnv *env, const char *className,
                                 jobjectArray *target) {
  char jniCls[100] = "jp/co/ntt/oss/heapstats/mbean/HeapStatsMBean$";
  strcat(jniCls, className);
  char sig[110];
  sprintf(sig, "()[L%s;", jniCls);

  jclass cls = loadClassGlobal(env, jniCls);
  if (cls == NULL) {
    return false;
  }

  jmethodID values = env->GetStaticMethodID(cls, "values", sig);
  if (values == NULL) {
    raiseException(env, "java/lang/RuntimeException",
                   "Could not get Enum values method.");
    return false;
  }

  *target = (jobjectArray)env->CallStaticObjectMethod(cls, values);
  if (*target != NULL) {
    *target = (jobjectArray)env->NewGlobalRef(*target);

    if (*target == NULL) {
      raiseException(env, "java/lang/RuntimeException",
                     "Could not get JNI Global value.");
      return false;
    }

  } else {
    raiseException(env, "java/lang/RuntimeException",
                   "Could not get Enum values.");
    return false;
  }

  return true;
}

/*!
 * \brief Preparation for number object.
 *        This function can initialize Integer and Long.
 *
 * \param env       Pointer of JNI environment.
 * \param className Class name to initialize.
 *                  It must be "Integer" or "Long".
 * \param cls       jclass object to store its class.
 * \param valueOfMethod jmethodID to store "valueOf" method in its class.
 * \param valueMethod   jmethodID to store "intValue" or "longValue" method
 *                      in its class.
 * \return true if succeeded.
 */
static bool prepareForNumObject(JNIEnv *env, const char *className, jclass *cls,
                                jmethodID *valueOfMethod,
                                jmethodID *valueMethod) {
  char jniCls[30] = "java/lang/";
  strcat(jniCls, className);
  char sig[40];

  *cls = loadClassGlobal(env, jniCls);
  if (*cls == NULL) {
    return false;
  }

  if (strcmp(className, "Long") == 0) {
    sprintf(sig, "(J)L%s;", jniCls);
    *valueOfMethod = env->GetStaticMethodID(*cls, "valueOf", sig);
    *valueMethod = env->GetMethodID(*cls, "longValue", "()J");
  } else {
    sprintf(sig, "(I)L%s;", jniCls);
    *valueOfMethod = env->GetStaticMethodID(*cls, "valueOf", sig);
    *valueMethod = env->GetMethodID(*cls, "intValue", "()I");
  }

  if ((*valueOfMethod == NULL) || (*valueMethod == NULL)) {
    raiseException(env, "java/lang/RuntimeException",
                   "Could not get valueOf method.");
    return false;
  }

  return true;
}

/*!
 * \brief Register JNI functions in libheapstats.
 *
 * \param env Pointer of JNI environment.
 * \param cls Class of HeapStatsMBean implementation.
 */
JNIEXPORT void JNICALL RegisterHeapStatsNative(JNIEnv *env, jclass cls) {
  /* Regist JNI functions */
  JNINativeMethod methods[] = {
      {(char *)"getHeapStatsVersion0",
       (char *)"()Ljava/lang/String;",
       (void *)GetHeapStatsVersion},
      {(char *)"getConfiguration0",
       (char *)"(Ljava/lang/String;)Ljava/lang/Object;",
       (void *)GetConfiguration},
      {(char *)"getConfigurationList0",
       (char *)"()Ljava/util/Map;",
       (void *)GetConfigurationList},
      {(char *)"changeConfiguration0",
       (char *)"(Ljava/lang/String;Ljava/lang/Object;)Z",
       (void *)ChangeConfiguration},
      {(char *)"invokeLogCollection0",
       (char *)"()Z",
       (void *)InvokeLogCollection},
      {(char *)"invokeAllLogCollection0",
       (char *)"()Z",
       (void *)InvokeAllLogCollection}};

  if (env->RegisterNatives(cls, methods, 6) != 0) {
    raiseException(env, "java/lang/UnsatisfiedLinkError",
                   "Native function for HeapStatsMBean failed.");
    return;
  }

  /* Initialize variables. */

  /* For Map object */
  if (!prepareForMapObject(env)) {
    return;
  }

  /* For Boolean object */
  if (!prepareForBoolean(env, "TRUE", &boolTrue) ||
      !prepareForBoolean(env, "FALSE", &boolFalse)) {
    return;
  }

  /* For Enum values */
  if (!prepareForEnumObject(env, "LogLevel", &logLevelArray) ||
      !prepareForEnumObject(env, "RankOrder", &rankOrderArray)) {
    return;
  }

  /* For number Object */
  if (!prepareForNumObject(env, "Integer", &integerCls, &intValueOf,
                           &intValue) ||
      !prepareForNumObject(env, "Long", &longCls, &longValueOf, &longValue)) {
    return;
  }
}

/*!
 * \brief Get HeapStats version string from libheapstats.
 *
 * \param env Pointer of JNI environment.
 * \param obj Instance of HeapStatsMBean implementation.
 * \return Version string which is attached.
 */
JNIEXPORT jstring JNICALL GetHeapStatsVersion(JNIEnv *env, jobject obj) {
  jstring versionStr = env->NewStringUTF(PACKAGE_STRING
                                         " ("
#ifdef SSE2
                                         "SSE2)"
#endif
#ifdef SSE3
                                         "SSE3)"
#endif
#ifdef SSE4
                                         "SSE4)"
#endif
#ifdef AVX
                                         "AVX)"
#endif
#if (!defined AVX) && (!defined SSE4) && (!defined SSE3) && (!defined SSE2)
                                         "None)"
#endif
                                         );

  if (versionStr == NULL) {
    raiseException(env, "java/lang/RuntimeException",
                   "Could not create HeapStats version string.");
  }

  return versionStr;
}

/*!
 * \brief Create java.lang.String instance.
 *
 * \param env Pointer of JNI environment.
 * \param val Value of string.
 * \return Instance of String.
 */
static jstring createString(JNIEnv *env, const char *val) {
  if (val == NULL) {
    return NULL;
  }

  jstring ret = env->NewStringUTF(val);
  if (ret == NULL) {
    raiseException(env, "java/lang/RuntimeException",
                   "Cannot get string in JNI");
  }

  return ret;
}

/*!
 * \brief Get configuration value as jobject.
 *
 * \param env    Pointer of JNI environment.
 * \param config Configuration element.
 * \return jobject value of configuration.
 */
static jobject getConfigAsJObject(JNIEnv *env, TConfigElementSuper *config) {
  jobject ret = NULL;

  switch (config->getConfigDataType()) {
    case BOOLEAN:
      ret = ((TBooleanConfig *)config)->get() ? boolTrue : boolFalse;
      break;
    case INTEGER:
      ret = env->CallStaticObjectMethod(integerCls, intValueOf,
                                        ((TIntConfig *)config)->get());
      break;
    case LONG:
      ret = env->CallStaticObjectMethod(longCls, longValueOf,
                                        ((TLongConfig *)config)->get());
      break;
    case STRING:
      ret = createString(env, ((TStringConfig *)config)->get());
      break;
    case LOGLEVEL:
      ret = env->GetObjectArrayElement(logLevelArray,
                                       ((TLogLevelConfig *)config)->get());
      break;
    case RANKORDER:
      ret = env->GetObjectArrayElement(rankOrderArray,
                                       ((TRankOrderConfig *)config)->get());
      break;
  }

  return ret;
}

/*!
 * \brief Get HeapStats agent configuration from libheapstats.
 *
 * \param env Pointer of JNI environment.
 * \param obj Instance of HeapStatsMBean implementation.
 * \param key Name of configuration.
 * \return Current value of configuration key.
 */
JNIEXPORT jobject JNICALL
    GetConfiguration(JNIEnv *env, jobject obj, jstring key) {
  const char *opt = env->GetStringUTFChars(key, NULL);
  if (opt == NULL) {
    raiseException(env, "java/lang/RuntimeException",
                   "Cannot get string in JNI");
    return NULL;
  }

  jobject result = NULL;
  std::list<TConfigElementSuper *> configs = conf->getConfigs();

  for (std::list<TConfigElementSuper *>::iterator itr = configs.begin();
       itr != configs.end(); itr++) {
    if (strcmp(opt, (*itr)->getConfigName()) == 0) {
      result = getConfigAsJObject(env, *itr);
      jthrowable ex = env->ExceptionOccurred();

      if (ex != NULL) {
        env->Throw(ex);
        return NULL;
      }
    }
  }

  env->ReleaseStringUTFChars(key, opt);
  return result;
}

/*!
 * \brief Get all of HeapStats agent configurations from libheapstats.
 *
 * \param env Pointer of JNI environment.
 * \param obj Instance of HeapStatsMBean implementation.
 * \return Current configuration list.
 */
JNIEXPORT jobject JNICALL GetConfigurationList(JNIEnv *env, jobject obj) {
  jobject result = env->NewObject(mapCls, map_ctor);
  if (result == NULL) {
    raiseException(env, "java/lang/RuntimeException",
                   "Cannot create Map instance.");
    return NULL;
  }

  std::list<TConfigElementSuper *> configs = conf->getConfigs();
  jstring key;
  jobject value = NULL;

  for (std::list<TConfigElementSuper *>::iterator itr = configs.begin();
       itr != configs.end(); itr++) {
    key = createString(env, (*itr)->getConfigName());
    if (key == NULL) {
      return NULL;
    }

    value = getConfigAsJObject(env, *itr);

    env->CallObjectMethod(result, map_put, key, value);
    if (env->ExceptionCheck()) {
      raiseException(env, "java/lang/RuntimeException",
                     "Cannot put config to Map instance.");
      return NULL;
    }
  }

  return result;
}

/*!
 * \brief Change HeapStats agent configuration in libheapstats.
 *
 * \param env   Pointer of JNI environment.
 * \param obj   Instance of HeapStatsMBean implementation.
 * \param key   Name of configuration.
 * \param value New configuration value.
 * \return Result of this change.
 */
JNIEXPORT jboolean JNICALL
    ChangeConfiguration(JNIEnv *env, jobject obj, jstring key, jobject value) {
  jclass valueCls = env->GetObjectClass(value);
  char *opt = (char *)env->GetStringUTFChars(key, NULL);
  if (opt == NULL) {
    raiseException(env, "java/lang/RuntimeException",
                   "Cannot get string in JNI");
    return JNI_FALSE;
  }

  TConfiguration new_conf = *conf;
  std::list<TConfigElementSuper *> configs = new_conf.getConfigs();

  for (std::list<TConfigElementSuper *>::iterator itr = configs.begin();
       itr != configs.end(); itr++) {
    if (strcmp(opt, (*itr)->getConfigName()) == 0) {
      switch ((*itr)->getConfigDataType()) {
        case BOOLEAN:
          if (env->IsAssignableFrom(valueCls, boolCls)) {
            jboolean newval = env->CallBooleanMethod(value, boolValue);
            ((TBooleanConfig *)*itr)->set((bool)newval);
          } else {
            raiseException(env, "java/lang/ClassCastException",
                           "Cannot convert new configuration to Boolean.");
          }
          break;
        case INTEGER:
          if (env->IsAssignableFrom(valueCls, integerCls)) {
            jint newval = env->CallIntMethod(value, intValue);
            ((TIntConfig *)*itr)->set(newval);
          } else {
            raiseException(env, "java/lang/ClassCastException",
                           "Cannot convert new configuration to Integer.");
          }
          break;
        case LONG:
          if (env->IsAssignableFrom(valueCls, longCls)) {
            jlong newval = env->CallLongMethod(value, longValue);
            ((TLongConfig *)*itr)->set(newval);
          } else {
            raiseException(env, "java/lang/ClassCastException",
                           "Cannot convert new configuration to Long.");
          }
          break;
        case LOGLEVEL:
          for (int Cnt = 0; Cnt < env->GetArrayLength(logLevelArray); Cnt++) {
            if (env->IsSameObject(
                    env->GetObjectArrayElement(logLevelArray, Cnt), value)) {
              ((TLogLevelConfig *)*itr)->set((TLogLevel)Cnt);
              break;
            }
          }
          break;
        case RANKORDER:
          for (int Cnt = 0; Cnt < env->GetArrayLength(rankOrderArray); Cnt++) {
            if (env->IsSameObject(
                    env->GetObjectArrayElement(rankOrderArray, Cnt), value)) {
              ((TRankOrderConfig *)*itr)->set((TRankOrder)Cnt);
              break;
            }
          }
          break;
        default:  // String

          if (value == NULL) {
            ((TStringConfig *)*itr)->set(NULL);
          } else if (env->IsAssignableFrom(
                         valueCls, env->FindClass("java/lang/String"))) {
            char *val = (char *)env->GetStringUTFChars((jstring)value, NULL);

            if (val == NULL) {
              raiseException(env, "java/lang/RuntimeException",
                             "Cannot get string in JNI");
            } else {
              ((TStringConfig *)*itr)->set(val);
              env->ReleaseStringUTFChars((jstring)value, val);
            }

          } else {
            raiseException(env, "java/lang/RuntimeException",
                           "Cannot support this configuration type.");
          }
      }
    }
  }

  jboolean result = JNI_FALSE;

  if (!env->ExceptionOccurred()) {
    if (new_conf.validate()) {
      conf->merge(&new_conf);
      logger->printInfoMsg("Configuration has been changed through JMX.");
      conf->printSetting();
      result = JNI_TRUE;
    } else {
      raiseException(env, "java/lang/IllegalArgumentException",
                     "Illegal parameter was set.");
    }
  }

  env->ReleaseStringUTFChars(key, opt);
  return result;
}

/*!
 * \brief Invoke Resource Log collection at libheapstats.
 *
 * \param env   Pointer of JNI environment.
 * \param obj   Instance of HeapStatsMBean implementation.
 * \return Result of this call.
 */
JNIEXPORT jboolean JNICALL InvokeLogCollection(JNIEnv *env, jobject obj) {
  int ret =
      logManager->collectLog(NULL, env, Signal, (TMSecTime)getNowTimeSec());
  return ret == 0 ? JNI_TRUE : JNI_FALSE;
}

/*!
 * \brief Invoke All Log collection at libheapstats.
 *
 * \param env   Pointer of JNI environment.
 * \param obj   Instance of HeapStatsMBean implementation.
 * \return Result of this call.
 */
JNIEXPORT jboolean JNICALL InvokeAllLogCollection(JNIEnv *env, jobject obj) {
  int ret = logManager->collectLog(NULL, env, AnotherSignal,
                                   (TMSecTime)getNowTimeSec());
  return ret == 0 ? JNI_TRUE : JNI_FALSE;
}
