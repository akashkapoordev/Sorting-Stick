#include "Gameplay/Collection/StickCollectionController.h"
#include "Gameplay/Collection/StickCollectionView.h"
#include "Gameplay/Collection/StickCollectionModel.h"
#include "Gameplay/GameplayService.h"
#include "Global/ServiceLocator.h"
#include "Gameplay/Collection/Stick.h"
#include <random>
#include <iostream>

namespace Gameplay
{
	namespace Collection
	{
		using namespace UI::UIElement;
		using namespace Global;
		using namespace Graphics;

		StickCollectionController::StickCollectionController()
		{
			collection_view = new StickCollectionView();
			collection_model = new StickCollectionModel();

			for (int i = 0; i < collection_model->number_of_elements; i++) sticks.push_back(new Stick(i));
		}

		StickCollectionController::~StickCollectionController()
		{
			destroy();
		}

		void StickCollectionController::initialize()
		{
			sort_state = SortState::NOT_SORTING;
			collection_view->initialize(this);
			initializeSticks();
			reset();
		}

		void StickCollectionController::initializeSticks()
		{
			float rectangle_width = calculateStickWidth();


			for (int i = 0; i < collection_model->number_of_elements; i++)
			{
				float rectangle_height = calculateStickHeight(i); //calc height

				sf::Vector2f rectangle_size = sf::Vector2f(rectangle_width, rectangle_height);

				sticks[i]->stick_view->initialize(rectangle_size, sf::Vector2f(0, 0), 0, collection_model->element_color);
			}
		}

		void StickCollectionController::update()
		{
			processSortThreadState();
			collection_view->update();
			for (int i = 0; i < sticks.size(); i++) sticks[i]->stick_view->update();
		}

		void StickCollectionController::render()
		{
			collection_view->render();
			for (int i = 0; i < sticks.size(); i++) sticks[i]->stick_view->render();
		}

		float StickCollectionController::calculateStickWidth()
		{
			// Get current screen width
			float total_space = static_cast<float>(ServiceLocator::getInstance()->getGraphicService()->getGameWindow()->getSize().x);

			// Reference resolution to scale
			sf::Vector2f reference_resolution = ServiceLocator::getInstance()->getGraphicService()->getReferenceResolution();

			// Calculate scaling factor based on current and reference resolution
			float scaling_factor = total_space / reference_resolution.x;

			// Calculate total spacing (e.g., 10% of the total space)
			float total_spacing = collection_model->space_percentage * total_space;

			// Calculate spacing between sticks
			float space_between = total_spacing / (collection_model->number_of_elements - 1);
			collection_model->setElementSpacing(space_between);

			// Calculate remaining space for the sticks
			float remaining_space = total_space - total_spacing;

			// Calculate rectangle width, ensuring it scales with resolution
			float rectangle_width = remaining_space / collection_model->number_of_elements;

			// Scale width and spacing if total width exceeds screen width
			float total_width = (rectangle_width * collection_model->number_of_elements) +
				(space_between * (collection_model->number_of_elements - 1));
			if (total_width > total_space)
			{
				float scale_factor = total_space / total_width;
				rectangle_width *= scale_factor;
				collection_model->setElementSpacing(space_between * scale_factor);
			}

			std::cout << "Screen Width: " << total_space << ", Total Spacing: " << total_spacing
				<< ", Rectangle Width: " << rectangle_width << ", Total Width: " << total_width << std::endl;

			return rectangle_width;
		}



		float StickCollectionController::calculateStickHeight(int array_pos)
		{
			return (static_cast<float>(array_pos + 1) / collection_model->number_of_elements) * collection_model->max_element_height;
		}

		void StickCollectionController::updateStickPosition()
		{
			float screen_width = static_cast<float>(ServiceLocator::getInstance()->getGraphicService()->getGameWindow()->getSize().x);

			// Total width available for sticks, excluding spacing
			float total_stick_width = screen_width * (1.0f - collection_model->space_percentage);

			// Calculate stick width and spacing proportionally
			float rectangle_width = total_stick_width / collection_model->number_of_elements;
			float spacing = (screen_width - total_stick_width) / (collection_model->number_of_elements + 1);

			for (int i = 0; i < sticks.size(); i++)
			{
				// Calculate positions proportionally
				float x_position = spacing + i * (rectangle_width + spacing);
				float y_position = collection_model->element_y_position - sticks[i]->stick_view->getSize().y;

				sticks[i]->stick_view->setPosition(sf::Vector2f(x_position, y_position));
			}
		}


		void StickCollectionController::shuffleSticks()
		{
			std::random_device device;
			std::mt19937 random_engine(device());

			std::shuffle(sticks.begin(), sticks.end(), random_engine);
			updateStickPosition();
		}

		bool StickCollectionController::compareSticksByData(const Stick* a, const Stick* b) const
		{
			return a->data < b->data;
		}

		void StickCollectionController::processSortThreadState()
		{
			if (sort_thread.joinable() && isCollectionSorted())
			{
				sort_thread.join();
				sort_state = Collection::SortState::NOT_SORTING;

			}
		}


		void StickCollectionController::resetSticksColor()
		{
			for (int i = 0; i < sticks.size(); i++) sticks[i]->stick_view->setFillColor(collection_model->element_color);
		}

		void StickCollectionController::resetVariables()
		{
			number_of_comparisons = 0;
			number_of_array_access = 0;
		}

		void StickCollectionController::reset()
		{
			color_delay = 0;
			sort_state = Collection::SortState::NOT_SORTING;
			current_operation_delay = 0;
			if (sort_thread.joinable()) sort_thread.join();

			shuffleSticks();
			resetSticksColor();
			resetVariables();
		}

		void StickCollectionController::sortElements(SortType sort_type)
		{
			color_delay = collection_model->color_delay;
			current_operation_delay = collection_model->operation_delay;
			this->sort_type = sort_type;
			sort_state = Gameplay::Collection::SortState::SORTING;

			switch (sort_type)
			{
			case Gameplay::Collection::SortType::BUBBLE_SORT:
				sort_thread = std::thread(&StickCollectionController::processBubbleSort, this);
				time_complexity = "O(n^2)";
				break;
			case Gameplay::Collection::SortType::INSERTION_SORT:
				sort_thread = std::thread(&StickCollectionController::processInsertionSort, this);
			}
			
		}

		bool StickCollectionController::isCollectionSorted()
		{
			for (int i = 1; i < sticks.size(); i++) if (sticks[i] < sticks[i - 1]) return false;
			return true;
		}

		void StickCollectionController::destroy()
		{
			current_operation_delay = 0;
			if (sort_thread.joinable()) sort_thread.join();

			for (int i = 0; i < sticks.size(); i++) delete(sticks[i]);
			sticks.clear();

			delete (collection_view);
			delete (collection_model);
		}

		SortType StickCollectionController::getSortType() { return sort_type; }

		int StickCollectionController::getNumberOfComparisons() { return number_of_comparisons; }

		int StickCollectionController::getNumberOfArrayAccess() { return number_of_array_access; }

		int StickCollectionController::getNumberOfSticks() { return collection_model->number_of_elements; }

		int StickCollectionController::getDelayMilliseconds() { return current_operation_delay; }

		sf::String StickCollectionController::getTimeComplexity() { return time_complexity; }

		void StickCollectionController::setCompletedColor()
		{
			for (int i = 0; i < sticks.size(); i++)
			{
				if (sort_state == Collection::SortState::NOT_SORTING) break;
				sticks[i]->stick_view->setFillColor(collection_model->element_color);

			}
			ServiceLocator::getInstance()->getSoundService()->playSound(Sound::SoundType::COMPARE_SFX);

			for (int i = 0; i < sticks.size(); i++)
			{
				if (sort_state == Collection::SortState::NOT_SORTING) break;
				ServiceLocator::getInstance()->getSoundService()->playSound(Sound::SoundType::COMPARE_SFX);
				std::this_thread::sleep_for(std::chrono::milliseconds(current_operation_delay));
				sticks[i]->stick_view->setFillColor(collection_model->placement_position_element_color);

			}

		}

		void StickCollectionController::processBubbleSort()
		{
			int length = sticks.size();
			bool swapped;

			do
			{
				swapped = false;
				for (int i = 1; i < length; i++)
				{
					if (sort_state == Collection::SortState::NOT_SORTING) break;
					number_of_array_access += 2;
					number_of_comparisons++;
					sticks[i]->stick_view->setFillColor(collection_model->processing_element_color);
					sticks[i - 1]->stick_view->setFillColor(collection_model->processing_element_color);
					ServiceLocator::getInstance()->getSoundService()->playSound(Sound::SoundType::COMPARE_SFX);
					if (sticks[i - 1]->data > sticks[i]->data)
					{
						Stick* temp = sticks[i];
						sticks[i] = sticks[i - 1];
						sticks[i - 1] = temp;
						swapped = true;
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(current_operation_delay));
					sticks[i]->stick_view->setFillColor(collection_model->element_color);
					sticks[i - 1]->stick_view->setFillColor(collection_model->element_color);
					updateStickPosition();
				}
				sticks[length-1]->stick_view->setFillColor(collection_model->placement_position_element_color);
				
				length--;
			} while (swapped);

			setCompletedColor();
		}
		void StickCollectionController::processInsertionSort()
		{
			for (int i = 1; i < sticks.size(); i++)
			{
				int j = i - 1;
				Stick* key = sticks[i];
				number_of_array_access++;
				key->stick_view->setFillColor(collection_model->processing_element_color);
				std::this_thread::sleep_for(std::chrono::milliseconds(current_operation_delay));

				while (j >= 0 && sticks[j]->data > key->data)
				{
					if (sort_state == SortState::NOT_SORTING) break;
					number_of_array_access++;
					number_of_comparisons++;
					sticks[j + 1] = sticks[j];
					sticks[j + 1]->stick_view->setFillColor(collection_model->processing_element_color);
					ServiceLocator::getInstance()->getSoundService()->playSound(Sound::SoundType::COMPARE_SFX);
					updateStickPosition();
					std::this_thread::sleep_for(std::chrono::milliseconds(current_operation_delay));
					sticks[j + 1]->stick_view->setFillColor(collection_model->selected_element_color);
					j--;

				}

				sticks[j + 1] = key;
				//sticks[j+1]->stick_view->setFillColor(collection_model->temporary_elemrnt_color);
				updateStickPosition();
				std::this_thread::sleep_for(std::chrono::milliseconds(current_operation_delay));
				sticks[j+1]->stick_view->setFillColor(collection_model->selected_element_color);

			}

			setCompletedColor();
		}
	}
}


