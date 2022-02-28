/* Copyright 2017-2021 Institute for Automation of Complex Power Systems,
 *                     EONERC, RWTH Aachen University
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *********************************************************************************/

#pragma once
#include <iostream>

#include <cps/Definitions.h>
#include <cps/PtrFactory.h>
#include <cps/MathUtils.h>
#include <cps/Config.h>
namespace CPS {
	template<class U>
	concept Arithmetic = std::is_arithmetic<U>::value;

	enum UpdateTaskKind {
		UPDATE_ONCE,
		UPDATE_ON_GET,
		UPDATE_ON_SET,
		UPDATE_ON_SIMULATION_STEP,
	};
	
	class AttributeBase {
	public:
		typedef std::shared_ptr<AttributeBase> Ptr;
		typedef std::vector<Ptr> List;
		typedef std::map<String, Ptr> Map;

		virtual String toString() = 0;
		virtual bool isStatic() const = 0;
		virtual AttributeBase::List getDependencies() = 0;
	};

	template<class T>
	class Attribute;

	template<class T>
	class AttributeStatic;

	template<class T>
	class AttributeDynamic;

	template<class DependentType>
	/// FIXME: Why cant this class be abstract?
	class AttributeUpdateTaskBase {

	public:
		virtual void executeUpdate(std::shared_ptr<DependentType> &dependent) {
			throw TypeException();
		};
		virtual AttributeBase::List getDependencies() {
			throw TypeException();
		};
	};

	template<class DependentType, class... DependencyTypes>
	class AttributeUpdateTask :
		public AttributeUpdateTaskBase<DependentType>,
		public SharedFactory<AttributeUpdateTask<DependentType, DependencyTypes...>> {
	
	public:
		using Actor = std::function<void(std::shared_ptr<DependentType>&, std::shared_ptr<Attribute<DependencyTypes>>...)>;

	protected:
		std::tuple<std::shared_ptr<Attribute<DependencyTypes>>...> mDependencies;
		Actor mActorFunction;
		UpdateTaskKind mKind;

	public:
		AttributeUpdateTask(UpdateTaskKind kind, Actor &actorFunction, std::shared_ptr<Attribute<DependencyTypes>>... dependencies)
			: mDependencies(std::forward<std::shared_ptr<Attribute<DependencyTypes>>>(dependencies)...), mActorFunction(actorFunction), mKind(kind) {}

		virtual void executeUpdate(std::shared_ptr<DependentType> &dependent) override {
			mActorFunction(dependent, std::get<std::shared_ptr<Attribute<DependencyTypes>>...>(mDependencies));
		}

		virtual AttributeBase::List getDependencies() override {
			return std::apply([](auto&&... elems){
				return std::vector<AttributeBase::Ptr>{std::forward<decltype(elems)>(elems)...};
			}, mDependencies);
		};
	};

	template<class T>
	class Attribute :
		public AttributeBase,
		public std::enable_shared_from_this<Attribute<T>> {

	protected:
		std::shared_ptr<T> mData;

	public:
		typedef T Type;
		typedef std::shared_ptr<Attribute<T>> Ptr;

		Attribute(T initialValue = T()) :
			AttributeBase(), mData(std::make_shared<T>()) {
				*mData = initialValue;
			}

		static Attribute<T>::Ptr create(String name, AttributeBase::Map &attrMap, T intitialValue = T()) {
			Attribute<T>::Ptr newAttr = AttributeStatic<T>::make(intitialValue);
			attrMap[name] = newAttr;
			return newAttr;
		}

		static Attribute<T>::Ptr createDynamic(String name, AttributeBase::Map &attrMap) {
			Attribute<T>::Ptr newAttr = AttributeDynamic<T>::make();
			attrMap[name] = newAttr;
			return newAttr;
		}

		virtual void set(T value) = 0;

		virtual T& get() = 0;

		virtual void setReference(Attribute<T>::Ptr reference) = 0;

		virtual const std::shared_ptr<T> asRawPointer() {
			return this->mData;
		}

		// virtual void reset() {
		// 	// TODO: we might want to provide a default value via the constructor
		// 	T resetValue = T();

		// 	// Only states are resetted!
		// 	if (mFlags & Flags::state)
		// 		set(resetValue);
		// }

		virtual String toString() override {
			/// CHECK: How does this impact performance?
			std::stringstream ss;
			ss.precision(2);
			ss << this->get();
			return ss.str();
		}

		/// @brief User-defined cast operator
		///
		/// Allows attributes to be casted to their value type:
		///
		/// Real v = 1.2;
		/// auto a = Attribute<Real>(&v);
		///
		/// Real x = v;
		///
		operator const T&() {
			return this->get();
		}

		/// @brief User-defined dereference operator
		///
		/// Allows easier access to the attribute's underlying data
		T& operator*(){
			return this->get();
		}

		// /// Do not use!
		// /// Only used for Eigen Matrix - Sundials N_Vector interfacing in N_VSetArrayPointer
		// operator T&() {
		// 	return this->get()->get();
		// }

		template <class U>
		typename Attribute<U>::Ptr derive(
			typename AttributeUpdateTask<U, T>::Actor getter = typename AttributeUpdateTask<U, T>::Actor(),
			typename AttributeUpdateTask<U, T>::Actor setter = typename AttributeUpdateTask<U, T>::Actor()
		)
		{
			auto derivedAttribute = std::make_shared<AttributeDynamic<U>>();
			if (setter) {
				derivedAttribute->addTask(UpdateTaskKind::UPDATE_ON_SET, AttributeUpdateTask<U, T>(UpdateTaskKind::UPDATE_ON_SET, setter, this->shared_from_this()));
			}
			if (getter) {
				derivedAttribute->addTask(UpdateTaskKind::UPDATE_ON_GET, AttributeUpdateTask<U, T>(UpdateTaskKind::UPDATE_ON_GET, getter, this->shared_from_this()));
			}
			return derivedAttribute;
		}

		std::shared_ptr<Attribute<Real>> deriveReal()
			requires std::same_as<T, CPS::Complex>
		{
			AttributeUpdateTask<CPS::Real, CPS::Complex>::Actor getter = [](std::shared_ptr<Real> &dependent, std::shared_ptr<Attribute<Complex>> dependency) {
				*dependent = (**dependency).real();
			};
			AttributeUpdateTask<CPS::Real, CPS::Complex>::Actor setter = [](std::shared_ptr<Real> &dependent, std::shared_ptr<Attribute<Complex>> dependency) {
				CPS::Complex currentValue = dependency->get();
				currentValue.real(*dependent);
				dependency->set(currentValue);
			};
			return derive<CPS::Real>(getter, setter);
		}

		std::shared_ptr<Attribute<Real>> deriveImag()
			requires std::same_as<T, CPS::Complex>
		{
			AttributeUpdateTask<CPS::Real, CPS::Complex>::Actor getter = [](std::shared_ptr<Real> &dependent, Attribute<Complex>::Ptr dependency) {
				*dependent = (**dependency).imag();
			};
			AttributeUpdateTask<CPS::Real, CPS::Complex>::Actor setter = [](std::shared_ptr<Real> &dependent, Attribute<Complex>::Ptr dependency) {
				CPS::Complex currentValue = dependency->get();
				currentValue.imag(*dependent);
				dependency->set(currentValue);
			};
			return derive<CPS::Real>(getter, setter);
		}

		std::shared_ptr<Attribute<Real>> deriveMag()
			requires std::same_as<T, CPS::Complex>
		{
			AttributeUpdateTask<CPS::Real, CPS::Complex>::Actor getter = [](std::shared_ptr<Real> &dependent, Attribute<Complex>::Ptr dependency) {
				*dependent = Math::abs(**dependency);
			};
			AttributeUpdateTask<CPS::Real, CPS::Complex>::Actor setter = [](std::shared_ptr<Real> &dependent, Attribute<Complex>::Ptr dependency) {
				CPS::Complex currentValue = dependency->get();
				dependency->set(Math::polar(*dependent, Math::phase(currentValue)));
			};
			return derive<CPS::Real>(getter, setter);
		}

		std::shared_ptr<Attribute<Real>> derivePhase()
			requires std::same_as<T, CPS::Complex>
		{
			AttributeUpdateTask<CPS::Real, CPS::Complex>::Actor getter = [](std::shared_ptr<Real> &dependent, Attribute<Complex>::Ptr dependency) {
				*dependent = Math::phase(**dependency);
			};
			AttributeUpdateTask<CPS::Real, CPS::Complex>::Actor setter = [](std::shared_ptr<Real> &dependent, Attribute<Complex>::Ptr dependency) {
				CPS::Complex currentValue = dependency->get();
				dependency->set(Math::polar(Math::abs(currentValue), *dependent));
			};
			return derive<CPS::Real>(getter, setter);
		}

		std::shared_ptr<Attribute<T>> deriveScaled(T scale)
			requires std::same_as<T, CPS::Complex> || std::same_as<T, CPS::Real>
		{
			typename AttributeUpdateTask<T, T>::Actor getter = [scale](std::shared_ptr<T> &dependent, Attribute<T>::Ptr dependency) {
				*dependent = scale * (**dependency);
			};
			typename AttributeUpdateTask<T, T>::Actor setter = [scale](std::shared_ptr<T> &dependent, Attribute<T>::Ptr dependency) {
				dependency->set((*dependent) / scale);
			};
			return derive<T>(getter, setter);
		}

		template<class U>
		std::shared_ptr<Attribute<U>> deriveCoeff(typename CPS::MatrixVar<U>::Index row, typename CPS::MatrixVar<U>::Index column)
			requires std::same_as<T, CPS::MatrixVar<U>>
		{
			typename AttributeUpdateTask<U, T>::Actor getter = [row, column](std::shared_ptr<U> &dependent, Attribute<T>::Ptr dependency) {
				*dependent = (**dependency)(row, column);
			};
			typename AttributeUpdateTask<U, T>::Actor setter = [row, column](std::shared_ptr<U> &dependent, Attribute<T>::Ptr dependency) {
				CPS::MatrixVar<U> currentValue = dependency->get();
				currentValue(row, column) = *dependent;
				dependency->set(currentValue);
			};
			return derive<U>(getter, setter);
		}

	};

	template<class T>
	class AttributeStatic :
		public Attribute<T>,
		public SharedFactory<AttributeStatic<T>> { 
		friend class SharedFactory<AttributeStatic<T>>;

	public:
		AttributeStatic(T initialValue = T()) :
			Attribute<T>(initialValue) { }

		virtual void set(T value) override {
			*this->mData = value;
		};

		virtual T& get() override {
			return *this->mData;
		};

		virtual bool isStatic() const override {
			return true;
		}

		virtual AttributeBase::List getDependencies() override {
			return AttributeBase::List();
		}

		virtual void setReference(typename Attribute<T>::Ptr reference) override {
			throw TypeException();
		}
	};

	template<class T>
	class AttributeDynamic :
		public Attribute<T>,
		public SharedFactory<AttributeDynamic<T>> { 
		friend class SharedFactory<AttributeDynamic<T>>;

	protected:
		///FIXME: The UPDATE_ONCE tasks are currently never triggered. Maybe at start of simulation?
		std::vector<AttributeUpdateTaskBase<T>> updateTasksOnce;
		std::vector<AttributeUpdateTaskBase<T>> updateTasksOnGet;
		std::vector<AttributeUpdateTaskBase<T>> updateTasksOnSet;

	public:
		AttributeDynamic(T initialValue = T()) :
			Attribute<T>(initialValue) { }

		void addTask(UpdateTaskKind kind, AttributeUpdateTaskBase<T> task) {
			switch (kind) {
				case UpdateTaskKind::UPDATE_ONCE:
					updateTasksOnce.push_back(task);
				case UpdateTaskKind::UPDATE_ON_GET:
					updateTasksOnGet.push_back(task);
					break;
				case UpdateTaskKind::UPDATE_ON_SET:
					updateTasksOnSet.push_back(task);
					break;
				case UpdateTaskKind::UPDATE_ON_SIMULATION_STEP:
					throw InvalidArgumentException();
			};
		}

		void clearTasks(UpdateTaskKind kind) {
			switch (kind) {
				case UpdateTaskKind::UPDATE_ONCE:
					updateTasksOnce.clear();
				case UpdateTaskKind::UPDATE_ON_GET:
					updateTasksOnGet.clear();
					break;
				case UpdateTaskKind::UPDATE_ON_SET:
					updateTasksOnSet.clear();
					break;
				case UpdateTaskKind::UPDATE_ON_SIMULATION_STEP:
					throw InvalidArgumentException();
			};
		}

		void clearAllTasks() {
			updateTasksOnce.clear();
			updateTasksOnGet.clear();
			updateTasksOnSet.clear();
		}

		virtual void setReference(typename Attribute<T>::Ptr reference) override {
			typename AttributeUpdateTask<T, T>::Actor getter = [](std::shared_ptr<T> &dependent, typename Attribute<T>::Ptr dependency) {
				dependent = dependency->asRawPointer();
			};
			this->clearAllTasks();
			if(reference->isStatic()) {
				this->addTask(UpdateTaskKind::UPDATE_ONCE, AttributeUpdateTask<T, T>(UpdateTaskKind::UPDATE_ONCE, getter, this->shared_from_this()));
			} else {
				this->addTask(UpdateTaskKind::UPDATE_ON_GET, AttributeUpdateTask<T, T>(UpdateTaskKind::UPDATE_ON_GET, getter, this->shared_from_this()));
			}
		}

		virtual void set(T value) override {
			*this->mData = value;
			for(auto task : updateTasksOnSet) {
				task.executeUpdate(this->mData);
			}
		};

		virtual T& get() override {
			for(auto task : updateTasksOnGet) {
				task.executeUpdate(this->mData);
			}
			return *this->mData;
		};

		virtual bool isStatic() const override {
			return false;
		}

		virtual AttributeBase::List getDependencies() {
			AttributeBase::List deps = AttributeBase::List();
			for (auto task : updateTasksOnce) {
				AttributeBase::List taskDeps = task.getDependencies();
				deps.insert(deps.end(), taskDeps.begin(), taskDeps.end());
			}

			for (auto task : updateTasksOnGet) {
				AttributeBase::List taskDeps = task.getDependencies();
				deps.insert(deps.end(), taskDeps.begin(), taskDeps.end());
			}
			return deps;
		}
	};

	template<>
	String Attribute<Complex>::toString();

	template<>
	String Attribute<String>::toString();
}
