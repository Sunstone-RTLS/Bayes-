#ifndef _BAYES_FILTER_SCHEME
#define _BAYES_FILTER_SCHEME

/*
 * Bayes++ the Bayesian Filtering Library
 * Copyright (c) 2004 Michael Stevens
 * See accompanying Bayes++.html for terms and conditions of use.
 *
 * $Header$
 * $NoKeywords: $
 */

/*
 * Generic Filter
 *  Filter schemes vary in their constructor parameterisation
 *  Filter_scheme derives a generic filter with consistent constructor interface
 *
 *  Provides specialisations for all Bayesian_filter schemes
 */


/* Filter namespace */
namespace Bayesian_filter
{

template <class Scheme>
struct Filter_scheme : public Scheme
/*
 * A Generic Filter Scheme
 *  Class template to provide a consistent constructor interface
 *  Default for Kalman_state_filter
 */
{
	Filter_scheme(std::size_t x_size, std::size_t q_maxsize) :
		Kalman_state (x_size), Scheme (x_size)
	{}
};


// UD_filter specialisation
template <>
struct Filter_scheme<UD_scheme> : public UD_scheme
{
	Filter_scheme(std::size_t x_size, std::size_t q_maxsize) :
		Kalman_state (x_size), UD_scheme (x_size, q_maxsize)
	{}
};

// Information_scheme specialisation
template <>
struct Filter_scheme<Information_scheme> : public Information_scheme
{
	Filter_scheme(std::size_t x_size, std::size_t q_maxsize) :
		Kalman_state (x_size), Information_state (x_size),
		Information_scheme (x_size)
	{}
};

// Information_root_info_scheme specialisation
template <>
struct Filter_scheme<Information_root_info_scheme> : public Information_root_info_scheme
{
	Filter_scheme(std::size_t x_size, std::size_t q_maxsize) :
		Kalman_state (x_size), Information_state (x_size),
		Information_root_info_scheme (x_size)
	{}
};

// SIR_scheme specialisation, inconsistent constructor
template <>
struct Filter_scheme<SIR_scheme> : public SIR_scheme
{
	Filter_scheme(std::size_t x_size, std::size_t s_size, SIR_random& random_helper) :
		Sample_state (x_size, s_size),
		SIR_scheme (x_size, s_size, random_helper)
	{}
};

// SIR_kalman_scheme specialisation, inconsistent constructor
template <>
struct Filter_scheme<SIR_kalman_scheme> : public SIR_kalman_scheme
{
	Filter_scheme(std::size_t x_size, std::size_t s_size, SIR_random& random_helper) :
		Sample_state (x_size, s_size),
		Kalman_state (x_size),
		SIR_kalman_scheme (x_size, s_size, random_helper)
	{}
};


}//namespace
#endif
