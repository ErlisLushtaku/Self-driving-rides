#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <stdexcept> // std::runtime_error
#include <math.h>	//abs()
#include <stdlib.h> //exit()
#include <iostream> //cerr<<

#include "../include/some_functions_h.h"
#include "../include/data_set_h.h"
#include "../include/global_variables_h.h"
#include "../include/ride_h.h"

using namespace std;

vector<int> split_string_to_ints(string line) {
	stringstream ss_line(line);
	vector<int> values;
	for (int number; ss_line >> number;) {
		values.push_back(number);
	}
	return values;
}

unordered_map<int, vector<int>> read_solution_file() {
	unordered_map<int, vector<int>> fleets;
	ifstream wf(solution_path);
	if (wf.is_open()) {
		string line;
		int line_number = 0;   // INITIAL VALUE
		while (getline(wf, line)) {
			fleets.insert(make_pair(line_number++, split_string_to_ints(line)));
		}
		wf.close();
	}
	else {
		cerr << "\n\nCould not open solution file!\n\n";
		exit(EXIT_FAILURE);
	}
	return fleets;
}


score_calculator::score_calculator() {}

score_calculator::score_calculator(const data_set& ds, unordered_map<int, vector<int>>* fleets) {
	this->ds = ds;
	this->fleets = fleets;
	score = 0;
	get_score();
}

void score_calculator::save_changes() {
	ride_info.erase(previous_ride);
	for (auto it = ride_info_changes.begin(); it != ride_info_changes.end(); it++) {
		ride_info[it->first] = it->second;
	}
	vehicle_scores[vehicle_scores_change.first] = vehicle_scores_change.second;
	this->score = new_score;
}

// Fills vehicle_scores
int score_calculator::get_score() {
	int vehicle_index = 0;
	int vehicle_score = 0;
	for (auto it = fleets->begin(); it != fleets->end(); it++)
	{
		vehicle_index = it->first;
		vehicle_score = get_score_for_one_vehicle(vehicle_index);
		vehicle_scores.insert(make_pair(vehicle_index, vehicle_score));

		this->score += vehicle_score;
	}

	return score;
}

// Fills ride_info
int score_calculator::get_score_for_one_vehicle(const int& vehicle_index) {
	int vehicle_score = 0; // Contains the score of the current vehicle

	int ride_score = 0;
	int ride_index = 0;

	pair<int, int> position(0, 0); // Starting position of each vehicle
	int step = 0; // The starting step for each vehicle

	int starting_time = 0;
	int distance_from_start_to_finish = 0;
	for (int i = 1; i < fleets->at(vehicle_index).size(); i++)                  // Iterate for each ride of the current vehicle
	{
		ride_index = fleets->at(vehicle_index)[i];
		ride_score = get_score_for_one_ride(ride_index, position, step); // Position and step are passed by reference therefore this function changes their value

		distance_from_start_to_finish = abs(ds.rides[ride_index]->x - ds.rides[ride_index]->a) + abs(ds.rides[ride_index]->y - ds.rides[ride_index]->b);
		starting_time = step - distance_from_start_to_finish;
		if (step <= ds.T) {
			ride_info.insert(pair<int, pair<int, int>>(ride_index, make_pair(ride_score, starting_time)));
			vehicle_score += ride_score;
		}
		else {
			for (int j = i; j < fleets->at(vehicle_index).size(); j++) {
				ride_index = fleets->at(vehicle_index)[j];
				ride_info.insert(pair<int, pair<int, int>>(ride_index, make_pair(0, -1)));
			}
			break;
		}
	}

	return vehicle_score;
}


int score_calculator::get_score_for_one_ride(const int& ride_index, pair<int, int>& position, int& step) {
	int ride_score = 0; // Contains the score of the current ride
	ride* current_ride = ds.rides[ride_index];

	int a = current_ride->a;	int x = current_ride->x;	int earliest_start = current_ride->s;
	int b = current_ride->b;	int y = current_ride->y;	int latest_finish = current_ride->f;

	int distance_from_position_to_start = abs(a - position.first) + abs(b - position.second); // Distance from current position to the start intersection of the ride
	step += distance_from_position_to_start;

	if (step < earliest_start) // Including the waiting time until the earliest start
		step = earliest_start;

	int distance_from_start_to_finish = abs(x - a) + abs(y - b); // Distance from the start intersection to the finish intersection of the ride
	step += distance_from_start_to_finish;

	if (step <= latest_finish) {
		ride_score += distance_from_start_to_finish;
		if ((step - distance_from_start_to_finish) == earliest_start)
			ride_score += ds.B;
	}

	position = make_pair(x, y); // At the end of the ride the vehicle is located at the finish intersection

	return ride_score;
}


int score_calculator::score_from_rides() {
	int score = 0;
	for (auto it = ride_info.begin(); it != ride_info.end(); it++) {
		int ride_score = it->second.first;
		score += ride_score;
	}
	return score;
}

int score_calculator::score_from_vehicles() {
	int score = 0;
	for (auto it = vehicle_scores.begin(); it != vehicle_scores.end(); it++) {
		int vehicle_score = it->second;
		score += vehicle_score;
	}
	return score;
}

// Delta get_score
// Calculates the difference of score if only one ride is changed
int score_calculator::get_score(const int& vehicle_index, const int& previous_ride_index_in_vector, const int& new_ride) {
	this->previous_ride = fleets->at(vehicle_index)[previous_ride_index_in_vector + 1];

	int vehicle_score = vehicle_scores.at(vehicle_index);
	int new_score = this->score - vehicle_score;

	// Calculate starting position and step before the ride_to_be_replaced
	pair<int, int> position;
	int step;

	ride* prior_ride;
	if (previous_ride_index_in_vector > 0) {
		int prior_ride_index = fleets->at(vehicle_index)[previous_ride_index_in_vector];
		prior_ride = ds.rides[prior_ride_index];

		position = make_pair(prior_ride->x, prior_ride->y);
		step = ride_info[prior_ride_index].second + abs(prior_ride->x - prior_ride->a) + abs(prior_ride->y - prior_ride->b);
	}
	else {
		position = make_pair(0, 0);
		step = 0;
	}

	int new_vehicle_score = vehicle_score;
	int distance_from_start_to_finish = 0;
	int new_starting_time = 0;
	// Iterate starting from the ride_to_be_replaced, until we see that the following ride's starting time is the same as before
	for (int i = previous_ride_index_in_vector + 1; i < fleets->at(vehicle_index).size(); i++) {

		int ride_index = fleets->at(vehicle_index)[i];               //indeksi i ride it ne vektorin vector<*rides>
		int previous_ride_score = ride_info[ride_index].first;					//skori i ride it qe po dojm ne ndrru
		new_vehicle_score -= previous_ride_score;
		int new_ride_score = 0;

		if (i == previous_ride_index_in_vector + 1) {
			new_ride_score = get_score_for_one_ride(new_ride, position, step);
			distance_from_start_to_finish = abs(ds.rides[new_ride]->x - ds.rides[new_ride]->a) + abs(ds.rides[new_ride]->y - ds.rides[new_ride]->b);
		}
		else {
			new_ride_score = get_score_for_one_ride(ride_index, position, step);
			distance_from_start_to_finish = abs(ds.rides[ride_index]->x - ds.rides[ride_index]->a) + abs(ds.rides[ride_index]->y - ds.rides[ride_index]->b);
		}

		new_starting_time = step - distance_from_start_to_finish;
		// In case we are not working with the ride_to_be_replaced, check if the starting_time is the same as before.
		// If it is the same it means we don't have to calculate for the following rides
		if (i != previous_ride_index_in_vector + 1)
			if (new_starting_time == ride_info[ride_index].second) {
				new_vehicle_score += previous_ride_score;
				break;
			}

		if (step <= ds.T) {
			new_vehicle_score += new_ride_score;
			if (i != previous_ride_index_in_vector + 1)
				ride_info_changes[ride_index] = make_pair(new_ride_score, new_starting_time);
			else {
				ride_info_changes[new_ride] = make_pair(new_ride_score, new_starting_time);
			}
		}
		else {
			for (int j = i; j < fleets->at(vehicle_index).size(); j++) {
				ride_index = fleets->at(vehicle_index)[j];
				if (j != previous_ride_index_in_vector + 1)
					ride_info_changes[ride_index] = make_pair(0, -1);
				else {
					ride_info_changes[new_ride] = make_pair(0, -1);
				}
			}
			break;
		}
	}
	// new_vehicle_scores.at(vehicle_index) = new_vehicle_score;
	vehicle_scores_change = make_pair(vehicle_index, new_vehicle_score);
	new_score += new_vehicle_score;
	this->new_score = new_score;

	return new_score;
}

// Delta get_score
// Calculates the difference of score if only one ride is changed
//int get_score(const data_set& ds, const unordered_map<int, vector<int>>& fleets, const int& previous_score, const int& vehicle_index, const int& previous_ride, const int& new_ride) {
//
//	int vehicle_score = 0; // Contains the score of the vehicle
//	pair<int, int> position(0, 0); // Starting position of the vehicle
//	int step = 0; // The starting step for each vehicle
//
//	 vehicle_score = vehicle_scores[vehicle_index];
//	vehicle_score = get_score_for_one_vehicle(ds, fleets, vehicle_index);
//
//	int new_score = previous_score - vehicle_score;
//
//	int new_vehicle_score = 0;
//	position = make_pair(0, 0);
//	step = 0;
//
//	int ride_score = 0;
//	int ride_index = 0;
//
//	for (int i = 1; i < fleets.at(vehicle_index).size(); i++) // Iterate for each ride of the current vehicle
//	{
//		ride_index = fleets.at(vehicle_index)[i];
//
//		if (ride_index == previous_ride)
//			ride_score = get_score_for_one_ride(ds, new_ride, position, step);
//		else
//			ride_score = get_score_for_one_ride(ds, ride_index, position, step);
//
//		if (step <= ds.T) {
//			new_vehicle_score += ride_score;
//		}
//		else
//			break;
//	}
//
//	vehicle_scores[vehicle_index] = new_vehicle_score;
//	new_score += new_vehicle_score;
//
//	return new_score;
//}

int get_score_for_one_vehicle(const data_set& ds, const vector<int>& scoring_vehicle) {
	int score = 0;
	if (scoring_vehicle.size() == 1)
		return score;
	int time_passed = 0;
	int distance_from_zero_intersection = ds.rides.at(scoring_vehicle[1])->a + ds.rides.at(scoring_vehicle[1])->b;
	for (int i = 1; i < scoring_vehicle.size(); i++)
	{
		int distance_between_intersections = abs(ds.rides.at(scoring_vehicle[i])->a - ds.rides.at(scoring_vehicle[i])->x) + abs(ds.rides.at(scoring_vehicle[i])->b - ds.rides.at(scoring_vehicle[i])->y);
		if (i == 1)
		{
			if (ds.rides.at(scoring_vehicle[i])->s >= distance_from_zero_intersection) {
				time_passed = ds.rides.at(scoring_vehicle[i])->s + distance_between_intersections;
				score = ds.B + distance_between_intersections;
				continue;
			}
			time_passed = distance_between_intersections + distance_from_zero_intersection;
			if (time_passed > ds.rides.at(scoring_vehicle[i])->f)
				continue;
			score = distance_between_intersections;
			continue;
		}
		int distance_last_first = abs(ds.rides.at(scoring_vehicle[i])->a - ds.rides.at(scoring_vehicle[i - 1])->x) + abs(ds.rides.at(scoring_vehicle[i])->b - ds.rides.at(scoring_vehicle[i - 1])->y);
		time_passed += distance_last_first;
		if (ds.rides.at(scoring_vehicle[i])->s >= time_passed) {
			time_passed = ds.rides.at(scoring_vehicle[i])->s + distance_between_intersections;
			score += ds.B + distance_between_intersections;
			continue;
		}
		time_passed += distance_between_intersections;
		if (time_passed > ds.rides.at(scoring_vehicle[i])->f)
			continue;
		score += distance_between_intersections;
	}
	return score;
}
