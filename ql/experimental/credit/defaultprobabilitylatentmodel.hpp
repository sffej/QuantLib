/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2014 Jose Aparicio

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/license.shtml>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/
#ifndef quantlib_defaultp_latent_model_hpp
#define quantlib_defaultp_latent_model_hpp

#include <boost/dynamic_bitset.hpp>

#include <ql/experimental/credit/basket.hpp>
#include <ql/experimental/math/latentmodel.hpp>
#include <ql/experimental/math/gaussiancopulapolicy.hpp>

namespace QuantLib {

    // \todo Split inline bodies from declarations.

    /*! Default event Latent Model. 
     This is a model for correlated default events based on a generic Latent 
      Model. It models solely the default events in a portfolio, not making any 
      reference to severities, exposures, etc...
     The class is parametric on the Latent Model copula.
    */
    template<class copulaPolicy>
    class DefaultProbLM : public LatentModel<copulaPolicy> {
    protected:
        const boost::shared_ptr<Basket> basket_;
        const boost::shared_ptr<Pool> pool_;// could pass without this one
    public:
        
        // mention theres an implicit mapping order between variables and pool names, but this should be in derived classes at this level the pool concept should not appear

        /*!
        @param basket The basket of issuers on which to model defaults.
        @param idiosyncFctrsWeights Idiosyncratic weights, see Latent Model doc.
        @param quadOrder The order of the quadrature to integrate the 
            model (sole integrator by now).
        @param ini Copula initialization if any.

        \warning Baskets with realized defaults not tested/WIP.
        */
        DefaultProbLM(
            const boost::shared_ptr<Basket>& basket,
            const std::vector<std::vector<Real> >& idiosyncFctrsWeights,///////////////////////////////inits in th e copula!!!!????
            Size quadOrder,
            const typename copulaPolicy::initTraits& ini = 
                copulaPolicy::initTraits()
            ) 
        : basket_(basket), 
          pool_(basket->pool()), 
          LatentModel<copulaPolicy>(idiosyncFctrsWeights, quadOrder, ini) 
        { }
        /* \todo
            Add other constructors as in LatentModel for ease of use. (less 
            dimensions, factors, etcc...)
        */
    protected:
        /*! Returns the probability of default of a given name conditional on
        the realization of a given set of values of the model independent
        factors. The date at which the probability is given is implicit in the
        probability since theres not other time dependence in this model.
        @param prob Unconditional probability of default.
        @param iName desired name.
        @param mktFactors Value of LM independent factors.
        \warning Most often it is preferred to use the method below avoiding the
        cumulative inversion.
        */
        Probability conditionalDefaultProbability(Probability prob, Size iName,
            const std::vector<Real>& mktFactors) const {
            /*Avoid redundant call to minimum value inversion (might be \infty),
            and this independently of the copula function.
            */
            if (prob < 1.e-10) return 0.;// use library macro..!
            return conditionalDefaultProbabilityInvP(
 ///               copulaPolicy::inverseCumulativeY(prob, iName), 
                inverseCumulativeY(prob, iName), 
                iName, mktFactors);
        }
        /*! Returns the probability of default of a given name conditional on
        the realization of a given set of values of the model independent
        factors. The date at which the probability is given is implicit in the
        probability since theres not other time dependent in this model.
        Same intention as above but provides a performance opportunity, if the
        integration is along the market factors (as usually is) avoids computing
        the inverse of the probability on each call.
        @param invCumYProb Inverse cumul of the unconditional probability of 
          default, has to follow the same copula law for results to be coherent
        @param iName desired name.
        @param mktFactors Value of LM independent factors.
        */
        Probability conditionalDefaultProbabilityInvP(Real invCumYProb, 
            Size iName, 
            const std::vector<Real>& m) const {
            Real sumMs = 
                std::inner_product(factorWeights_[iName].begin(), 
                    factorWeights_[iName].end(), m.begin(), 0.);//SHOULD BE IN BASE CLASS 
            Real res = 
 ////////////////////////////               copulaPolicy::cumulativeZ((invCumYProb - sumMs) / 
                cumulativeZ((invCumYProb - sumMs) / 
                    idiosyncFctrs_[iName] );
        /* DISABLE TO TEST JACOBIAN*//////////////////////////////////////////////////////////////////////
            #if defined(QL_EXTRA_SAFETY_CHECKS)
            QL_REQUIRE (res >= 0. && res <= 1.,
                        "conditional probability " << res << "out of range");
            #endif
        
            return res;
        }
        /*! Returns the probability of default of a given name conditional on
        the realization of a given set of values of the model independent
        factors.
        @param date The date for the probability of default.
        @param iName desired name.
        @param mktFactors Value of LM independent factors.

        Same intention as the above methods. Usage of this one is typically more
        expensive because most often the date we call this method with
        repeats itself and with this one the probability can not be cached
        outside the call.
        */
        Probability conditionalDefaultProbability(const Date& date, Size iName,
            const std::vector<Real>& mktFactors) const {
            Probability pDefUncond =
                pool_->get(pool_->names()[iName]).
                defaultProbability(basket_->defaultKeys()[iName])
                  ->defaultProbability(date);
            return conditionalDefaultProbability(pDefUncond, iName, mktFactors);          
        }
    public:
        Real toto(const std::vector<Real>& m) const { return 1.;}///////////////////////////////////////////
        /*! Computes the unconditional probability of default of a given name. 
        Trivial method for testing
        */
        Probability probOfDefault(Size iName, const Date& d) const {
            // avoid repeating this in the integration:
            Probability pUncond = pool_->get(pool_->names()[iName]).
                defaultProbability(basket_->defaultKeys()[iName])
                ->defaultProbability(d);
            if (pUncond < 1.e-10) return 0.;
            Real pUncondInv = 
     //////////////////////////           copulaPolicy::inverseCumulativeY(pUncond, iName);
                inverseCumulativeY(pUncond, iName);

            ////return this->integrate(
            ////  boost::function<Real (const std::vector<Real>& v1)>(
            ////    boost::bind(
            ////&DefaultProbLM<copulaPolicy>::toto,////////////////////////////////////////
            ////    this,
            ////    _1)));

            return this->integrate(
              boost::function<Real (const std::vector<Real>& v1)>(
                boost::bind(
                &DefaultProbLM<copulaPolicy>::conditionalDefaultProbabilityInvP,
                this,
                pUncondInv,
                iName, 
                _1)
              ));
        }
        /*! Returns the probaility of having a given or larger number of 
        defaults in the basket portfolio at a given time.
        */
        Probability probAtLeastNEvents(Size n, const Date& date) const {
            return this->integrate(
                boost::function<Real (const std::vector<Real>& v1)>(
                    boost::bind(
                    &DefaultProbLM<copulaPolicy>::conditionalProbAtLeastNEvents,
                    this,
                    n,
                    boost::cref(date),
                    _1)
                ));
        }
    protected:
        //! Conditional probability of n default events or more.
        // \todo: check the issuer has not defaulted.
        Real conditionalProbAtLeastNEvents(Size n, const Date& date,
            const std::vector<Real>& mktFactors) const {
                /* \todo 
                This algorithm traverses all permutations starting form the
                lowest one. This is inneficient, there shouldnt be any need to 
                go through the invalid ones. Use combinations of n elements.

                For more efficient methods see other default latent models.
                */
                // first position with as many defaults as desired:
                Size poolSize = basket_->size();//SHOULD BE LIVE SIZE---
                
                BigNatural limit = 
                    static_cast<BigNatural>(std::pow(2., (int)(poolSize)));

                // Precalc conditional probabilities
                std::vector<Probability> pDefCond;
                for(Size i=0; i<poolSize; i++)
                    pDefCond.push_back(conditionalDefaultProbability(
                        pool_->get(pool_->names()[i]).
                        defaultProbability(basket_->defaultKeys()[i])->
                        defaultProbability(date), i, mktFactors));

                Probability probNEventsOrMore = 0.;
                for(BigNatural mask = 
                      static_cast<BigNatural>(std::pow(2., (int)(n))-1);
                    mask < limit; mask++) 
                {
                    // cheap permutations
                    boost::dynamic_bitset<> bsetMask(poolSize, mask);
                    if(bsetMask.count() >= n) {
                        Probability pConfig = 1;
                        for(Size i=0; i<bsetMask.size(); i++)
                            pConfig *= 
                              (bsetMask[i] ? pDefCond[i] : (1.- pDefCond[i]));
                        probNEventsOrMore += pConfig;
                    }
                }
                return probNEventsOrMore;
        }
    };

    // often used:
    typedef DefaultProbLM<GaussianCopulaPolicy> GaussianDefProbLM;
    typedef DefaultProbLM<TCopulaPolicy> TDefProbLM;


/*

    template<class copulaPolicy>
    class RandomDefaultProbLM : public LatentModel<copulaPolicy> {
    protected:
        const boost::shared_ptr<Basket> basket_;
        const boost::shared_ptr<Pool> pool_;// could pass without this one
    public:
        DefaultProbLM(
            const boost::shared_ptr<Basket>& basket,
            const std::vector<std::vector<Real> >& idiosyncFctrsWeights,///////////////////////////////inits in th e copula!!!!????
            Size quadOrder,
            const typename copulaPolicy::initTraits& ini = 
                copulaPolicy::initTraits()
            ) 
        : basket_(basket), 
          pool_(basket->pool()), 
          LatentModel<copulaPolicy>(idiosyncFctrsWeights, quadOrder, ini) 
        { }
    protected:
    public:
//..............///

        Probability probOfDefault(Size iName, const Date& d) const {
//..............///
        }
        Probability probAtLeastNEvents(Size n, const Date& date) const {
//..............///

        }

    };
*/
}

#endif
